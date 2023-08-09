/*
* lru for database cache
* do something for the query and query response packets
*/

#include<core.p4>
#if __TARGET_TOFINO__ == 2
#include<t2na.p4>
#else
#include<tna.p4>
#endif

/*************************************************************************
 ************* C O N S T A N T S    A N D   T Y P E S  *******************
*************************************************************************/
const bit<32> BUCKET_SIZE = 1<<16;
typedef bit<16> index_len_t;
typedef bit<32> b32_t;
typedef bit<16> b16_t;
typedef bit<8> b8_t;

/*************************************************************************
 ************************** H E A D E R S  *******************************
*************************************************************************/
header ethernet_h {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_h {
    bit<4> version;
    bit<4> ihl;
    bit<8> diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3> flags;
    bit<13> frag_offset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header udp_h {
	bit<16> src_port;
	bit<16> dst_port;
	bit<16> total_len;
	bit<16> checksum;
}

header record_h{
    bit<32> type;
    bit<32> key;
    bit<32> value;
}


struct ingress_metadata_t {
    bit<32> index;
    bit<32> key_1;
    bit<32> key_2;
    bit<32> key_3;

    bit<32> value;

    bit<8> dfa_tsm;
    bit<8> dfa_sta;
}

struct ingress_header_t {
	ethernet_h ethernet;
	ipv4_h ipv4;
	udp_h udp;
    record_h record;
}

struct egress_header_t {
	ethernet_h ethernet;
	ipv4_h ipv4;
	udp_h udp;
    record_h record;

}

struct egress_metadata_t {
}

/*************************************************************************
 **************I N G R E S S  P A R S E R  *******************************
*************************************************************************/
parser IngressParser1(
        packet_in pkt,
        out ingress_header_t hdr,
        out ingress_metadata_t meta,
        out ingress_intrinsic_metadata_t ig_intr_md)
{
	state start {
		pkt.extract(ig_intr_md);
		pkt.advance(PORT_METADATA_SIZE);
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.ether_type) {
			0x0800: parse_ipv4;
			default: reject;
		}		
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);
		transition select(hdr.ipv4.protocol) {
			17: parse_udp;
			default: accept;
		}
	}

	state parse_udp {
		pkt.extract(hdr.udp);
		transition parse_record;
	}

	state parse_record{
		pkt.extract(hdr.record);
		transition accept;
	}
	
}

/*************************************************************************
 ***********************  I N G R E S S  *********************************
*************************************************************************/
control Ingress1(inout ingress_header_t hdr,
		inout ingress_metadata_t meta,
		in ingress_intrinsic_metadata_t ig_intr_md,
		in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
		inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
		inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
{
/********************** PREPROCESSING *****************************/	
    Hash<index_len_t>(HashAlgorithm_t.CRC32) key_hash;
    action preprocessing() {
        meta.index = (b32_t)key_hash.get({hdr.record.key});
    }

    @stage(0)
    table preprocessing_table{
        actions = {
            preprocessing;
        }
        size = 1;
        const default_action = preprocessing;
    }


/********************** LRU KEY *****************************/	
/****************** KEY 1 *************************/	
    Register<b32_t, b32_t>(BUCKET_SIZE) key_1;
    RegisterAction<b32_t, b32_t, b32_t>(key_1) read_key_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(key_1) insert_key_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.key;
        }
    };

    action read_key_1() {
        meta.key_1 = read_key_salu_1.execute(meta.index);
    }

    action insert_key_1() {
        meta.key_1 = insert_key_salu_1.execute(meta.index);  
    }

    @stage(1)
    table read_key_table_1 {
        actions = {
            read_key_1;
        }
        size = 1;
        const default_action = read_key_1;
    }

    @stage(1)
    table insert_key_table_1 {
        actions = {
            insert_key_1;
        }
        size = 1;
        const default_action = insert_key_1;
    }


/*--------------------- KEY 2 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) key_2;
    RegisterAction<b32_t, b32_t, b32_t>(key_2) read_key_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(key_2) insert_key_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = meta.key_1;
        }
    };

    action read_key_2() {
        meta.key_2 = read_key_salu_2.execute(meta.index);
    }

    action insert_key_2() {
        meta.key_2 = insert_key_salu_2.execute(meta.index);  
    }

    @stage(3)
    table read_key_table_2 {
        actions = {
            read_key_2;
        }
        size = 1;
        const default_action = read_key_2;
    }

    @stage(3)
    table insert_key_table_2 {
        actions = {
            insert_key_2;
        }
        size = 1;
        const default_action = insert_key_2;
    }

/*--------------------- KEY 3 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) key_3;
    RegisterAction<b32_t, b32_t, b32_t>(key_3) read_key_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(key_3) insert_key_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = meta.key_2;
        }
    };


    action read_key_3() {
        meta.key_3 = read_key_salu_3.execute(meta.index);
    }

    action insert_key_3() {
        meta.key_3 = insert_key_salu_3.execute(meta.index);  
    }


    @stage(5)
    table read_key_table_3 {
        actions = {
            read_key_3;
        }
        size = 1;
        const default_action = read_key_3;
    }

    @stage(5)
    table insert_key_table_3 {
        actions = {
            insert_key_3;
        }
        size = 1;
        const default_action = insert_key_3;
    }
/********************** LRU DFA *****************************/
    Register<b8_t, b32_t>(BUCKET_SIZE) reg_dfa;
    RegisterAction<b8_t, b32_t, b8_t>(reg_dfa) dfa_salu_1 = {
        void apply (inout b8_t reg_data, out b8_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b8_t, b32_t, b8_t>(reg_dfa) dfa_salu_2 = {
        void apply (inout b8_t reg_data, out b8_t out_data) {
            if (reg_data >= 4) {
                reg_data = reg_data ^ 1;
            }
            else {
                reg_data = reg_data ^ 3;
            }
            out_data = reg_data;
        }
    };

    RegisterAction<b8_t, b32_t, b8_t>(reg_dfa) dfa_salu_3 = {
        void apply (inout b8_t reg_data, out b8_t out_data) {
            if (reg_data <= 1) {
                reg_data = reg_data + 4;
            }
            else {
                reg_data = reg_data - 2;
            }
            out_data = reg_data;
        }
    };

    action dfa_action_1() {
        meta.dfa_sta = dfa_salu_1.execute(meta.index);
    }

    action dfa_action_2() {
        meta.dfa_sta = dfa_salu_2.execute(meta.index);
    }

    action dfa_action_3() {
        meta.dfa_sta = dfa_salu_3.execute(meta.index);
    }

    @stage(5) 
    table dfa_table_1 {
        actions = {
            dfa_action_1;
        }
        size = 1;
        const default_action = dfa_action_1;
    }

    @stage(7) 
    table read_dfa_table_1 {
        actions = {
            dfa_action_1;
        }
        size = 1;
        const default_action = dfa_action_1;
    }

    @stage(7) 
    table dfa_table_2 {
        actions = {
            dfa_action_2;
        }
        size = 1;
        const default_action = dfa_action_2;
    }

    @stage(7) 
    table dfa_table_3 {
        actions = {
            dfa_action_3;
        }
        size = 1;
        const default_action = dfa_action_3;
    }

/********************** LRU VALUE *****************************/	
/*************** VALUE 1 ********************/	
    Register<b32_t, b32_t>(BUCKET_SIZE) value_1;
    RegisterAction<b32_t, b32_t, b32_t>(value_1) query_value_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            if(meta.dfa_tsm == 4) {
                out_data = 0;
                reg_data = 0;
            }
            else {
                out_data = reg_data;
            }
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(value_1) replace_value_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.value;
        }
    };

    action query_value_action_1() {
        meta.value = query_value_salu_1.execute(meta.index);
    }

    action replace_value_action_1() {
        meta.value = replace_value_salu_1.execute(meta.index);
    }

    @stage(9) 
    table query_value_table_1 {
        actions = {
            query_value_action_1;
        }
        size = 1;
        const default_action = query_value_action_1;
    }

    @stage(9) 
    table replace_value_table_1 {
        actions = {
            replace_value_action_1;
        }
        size = 1;
        const default_action = replace_value_action_1;
    }

/*************** VALUE 2 ********************/	
    Register<b32_t, b32_t>(BUCKET_SIZE) value_2;
    RegisterAction<b32_t, b32_t, b32_t>(value_2) query_value_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            if(meta.dfa_tsm == 4) {
                out_data = 0;
                reg_data = 0;
            }
            else {
                out_data = reg_data;
            }
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(value_2) replace_value_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.value;
        }
    };

    action query_value_action_2() {
        meta.value = query_value_salu_2.execute(meta.index);
    }

    action replace_value_action_2() {
        meta.value = replace_value_salu_2.execute(meta.index);
    }

    @stage(9) 
    table query_value_table_2 {
        actions = {
            query_value_action_2;
        }
        size = 1;
        const default_action = query_value_action_2;
    }

    @stage(9) 
    table replace_value_table_2 {
        actions = {
            replace_value_action_2;
        }
        size = 1;
        const default_action = replace_value_action_2;
    }

/*************** VALUE 3 ********************/	
    Register<b32_t, b32_t>(BUCKET_SIZE) value_3;
    RegisterAction<b32_t, b32_t, b32_t>(value_3) query_value_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            if(meta.dfa_tsm == 4) {
                out_data = 0;
                reg_data = 0;
            }
            else {
                out_data = reg_data;
            }
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(value_3) replace_value_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.value;
        }
    };

    action query_value_action_3() {
        meta.value = query_value_salu_3.execute(meta.index);
    }

    action replace_value_action_3() {
        meta.value = replace_value_salu_3.execute(meta.index);
    }

    @stage(10) 
    table query_value_table_3 {
        actions = {
            query_value_action_3;
        }
        size = 1;
        const default_action = query_value_action_3;
    }

    @stage(10) 
    table replace_value_table_3 {
        actions = {
            replace_value_action_3;
        }
        size = 1;
        const default_action = replace_value_action_3;
    }

/********************** LRU VALUE *****************************/	
    action send_back() {
        ig_tm_md.ucast_egress_port = ig_intr_md.ingress_port;
    }
    
    @stage(11)
    table send_back_table {
        actions = {
            send_back;
        }
        size = 1;
        const default_action = send_back;
    }

    action send(PortId_t port) {
        ig_tm_md.ucast_egress_port = port;
    }

    @stage(0)
    table send_table {
        key = {
            ig_intr_md.ingress_port:exact;
        }
        actions = {
            send;
        }
        size = 16;
    }

/********************** INGRESS LOGIC *****************************/	
	apply {
        preprocessing_table.apply();
        send_table.apply();
        if(hdr.udp.isValid()&&hdr.record.type == 0x0527) {      //query
            if (meta.dfa_tsm == 0) {
                insert_key_table_1.apply();
                if (meta.key_1 == hdr.record.key) {
                    meta.dfa_tsm = 1;
                }
            }
            if (meta.dfa_tsm == 0) {
                insert_key_table_2.apply();
                if (meta.key_2 == hdr.record.key) {
                    meta.dfa_tsm = 2;
                }
            }
            if(meta.dfa_tsm == 0) {
                insert_key_table_3.apply();
                if (meta.key_3 == hdr.record.key) {
                    meta.dfa_tsm = 3;
                } else {
                    meta.dfa_tsm = 4;
                }
            }
        }
        else if(hdr.udp.isValid()&&hdr.record.type == 0x0529) {    //query back
            if (meta.dfa_tsm == 0) {
                read_key_table_1.apply();
                if (meta.key_1 == hdr.record.key) {
                    meta.dfa_tsm = 1;
                }
            }
            if (meta.dfa_tsm == 0) {
                read_key_table_2.apply();
                if (meta.key_2 == hdr.record.key) {
                    meta.dfa_tsm = 2;
                }
            }
            if(meta.dfa_tsm == 0) {
                read_key_table_3.apply();
                if (meta.key_3 == hdr.record.key) {
                    meta.dfa_tsm = 3;
                } else {
                    meta.dfa_tsm = 4;
                }
            }
        }

        if(hdr.udp.isValid()&&hdr.record.type == 0x0527) {      //query
            if (meta.dfa_tsm == 1) {
                dfa_table_1.apply();
            } else if (meta.dfa_tsm == 2) {
                dfa_table_2.apply();
            } else {
                dfa_table_3.apply();
            }
        }
        else if(hdr.udp.isValid()&&hdr.record.type == 0x0529) {    //query back
            dfa_table_1.apply();
        }

        if(hdr.udp.isValid()&&hdr.record.type == 0x0527) {      //query
            if (meta.dfa_sta == 4 || meta.dfa_sta == 1) {
                query_value_table_1.apply();
            } else if (meta.dfa_sta == 5 || meta.dfa_sta == 0) {
                query_value_table_2.apply();
            } else {
                query_value_table_3.apply();
            }

            if(meta.value != 0) {
                hdr.record.type = 0x0529;
                hdr.record.value = meta.value;
                send_back_table.apply();
            }
        }
        else if(hdr.udp.isValid()&&hdr.record.type == 0x0529) {    //query back

            if(meta.dfa_tsm == 1 && meta.dfa_sta == 4) {
                replace_value_table_1.apply();
            }
            else if(meta.dfa_tsm == 1 && meta.dfa_sta == 1) {
                replace_value_table_1.apply();
            }
            else if(meta.dfa_tsm == 2 && meta.dfa_sta == 2) {
                replace_value_table_1.apply();
            }
            else if(meta.dfa_tsm == 2 && meta.dfa_sta == 5) {
                replace_value_table_1.apply();
            }
            else if(meta.dfa_tsm == 3 && meta.dfa_sta == 0) {
                replace_value_table_1.apply();
            }
            else if(meta.dfa_tsm == 3 && meta.dfa_sta == 3) {
                replace_value_table_1.apply();
            }
            else if(meta.dfa_tsm == 1 && meta.dfa_sta == 5) {
                replace_value_table_2.apply();
            }
            else if(meta.dfa_tsm == 1 && meta.dfa_sta == 0) {
                replace_value_table_2.apply();
            }
            else if(meta.dfa_tsm == 2 && meta.dfa_sta == 4) {
                replace_value_table_2.apply();
            }
            else if(meta.dfa_tsm == 2 && meta.dfa_sta == 3) {
                replace_value_table_2.apply();
            }
            else if(meta.dfa_tsm == 3 && meta.dfa_sta == 2) {
                replace_value_table_2.apply();
            }
            else if(meta.dfa_tsm == 3 && meta.dfa_sta == 1) {
                replace_value_table_2.apply();
            }
            else if(meta.dfa_tsm == 1 && meta.dfa_sta == 2) {
                replace_value_table_3.apply();
            }
            else if(meta.dfa_tsm == 1 && meta.dfa_sta == 3) {
                replace_value_table_3.apply();
            }
            else if(meta.dfa_tsm == 2 && meta.dfa_sta == 0) {
                replace_value_table_3.apply();
            }
            else if(meta.dfa_tsm == 2 && meta.dfa_sta == 1) {
                replace_value_table_3.apply();
            }
            else if(meta.dfa_tsm == 3 && meta.dfa_sta == 4) {
                replace_value_table_3.apply();
            }
            else if(meta.dfa_tsm == 3 && meta.dfa_sta == 5) {
                replace_value_table_3.apply();
            }
        }
    }
}

/*************************************************************************
 ***************  I N G R E S S  D E P A R S E R *************************
*************************************************************************/
control IngressDeparser(packet_out pkt,
	inout ingress_header_t hdr,
	in ingress_metadata_t meta,
	in ingress_intrinsic_metadata_for_deparser_t ig_dprtr_md)
{
	apply{
		pkt.emit(hdr);
	}
}


/*************************************************************************
 *********************  E G R E S S  P A R S E R *************************
*************************************************************************/
parser EgressParser(packet_in pkt,
	out egress_header_t hdr,
	out egress_metadata_t meta,
	out egress_intrinsic_metadata_t eg_intr_md)
{
	state start{
		pkt.extract(eg_intr_md);
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.ether_type) {
			0x0800: parse_ipv4;
			default: reject;
		}		
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);
		transition select(hdr.ipv4.protocol) {
			17: parse_udp;
			default: accept;
		}
	}

	state parse_udp {
		pkt.extract(hdr.udp);
		transition parse_record;
	}

	state parse_record{
		pkt.extract(hdr.record);
		transition accept;
	}
}

/*************************************************************************
 *****************************  E G R E S S ******************************
*************************************************************************/
control Egress1(inout egress_header_t hdr,
	inout egress_metadata_t meta,
	in egress_intrinsic_metadata_t eg_intr_md,
	in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
	inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
	inout egress_intrinsic_metadata_for_output_port_t eg_oport_md)
{
    apply{
    }
}
/*************************************************************************
 *********************  E G R E S S  D E P A R S E R *********************
*************************************************************************/
control EgressDeparser(packet_out pkt,
	inout egress_header_t hdr,
	in egress_metadata_t meta,
	in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md)
{
	apply{
        pkt.emit(hdr);
	}
}


/*************************************************************************
 ***************************  M A I N ************************************
*************************************************************************/
Pipeline(IngressParser1(),Ingress1(),IngressDeparser(),
EgressParser(),Egress1(),EgressDeparser()) pipe1;


Switch(pipe1) main;

