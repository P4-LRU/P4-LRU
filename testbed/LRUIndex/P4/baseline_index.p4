/*
* lru for database cache
* do something for the query and query response packets
*/

/*
* Ingress1: hash the key to decide which registers to store the info. implement the first part cache
* Ingress2: implement the second part cache
* Egress1: do nothing
* Egress2: do nothing
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
typedef bit<12> rand_len_t;
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
    bit<64> key;
    bit<32> addr_1;
    bit<16> addr_2;

    bit<8> position;
}

header bridge_h {
    bit<32> index;
    bit<32> key;
    bit<32> addr_1;
    bit<16> addr_2;
    bit<8> flag;
}


struct ingress_metadata_t {
    bit<32> key;
    bit<12> rand;
}

struct ingress_header_t {
	ethernet_h ethernet;
	ipv4_h ipv4;
	udp_h udp;
    record_h record;
    bridge_h bridge;
}

struct egress_header_t {
	ethernet_h ethernet;
	ipv4_h ipv4;
	udp_h udp;
    record_h record;
    bridge_h bridge;
}

struct egress_metadata_t {
    bit<32> key;
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
        hdr.bridge.setValid();
        hdr.bridge.index = 0;
        hdr.bridge.key = 0;
        hdr.bridge.addr_1 = 0;
        hdr.bridge.addr_2 = 0;
        hdr.bridge.flag = 0;
		transition accept;
	}
	
}

parser IngressParser2(
        packet_in pkt,
        out ingress_header_t hdr,
        out ingress_metadata_t metadata,
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
        pkt.extract(hdr.bridge);
		transition accept;
	}
	
}

/*************************************************************************
 ***********************  I N G R E S S  *********************************
*************************************************************************/

// hash the key to decide which registers to store the info. implement the first part cache
control Ingress1(inout ingress_header_t hdr,
		inout ingress_metadata_t meta,
		in ingress_intrinsic_metadata_t ig_intr_md,
		in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
		inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
		inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
{
/********************** PREPROCESSING *****************************/
	Hash<index_len_t>(HashAlgorithm_t.RANDOM) index_hash;
    Hash<b32_t>(HashAlgorithm_t.CRC32) key_hash;
    Hash<rand_len_t>(HashAlgorithm_t.CRC16) random_hash;

    action set_port_action(PortId_t port) {
        ig_tm_md.ucast_egress_port = port;
    }

    action preprocessing_1() {
        hdr.bridge.index = (b32_t)index_hash.get(hdr.record.key);
    }

    action preprocessing_2() {
        hdr.bridge.key = key_hash.get(hdr.record.key);
    }

    action preprocessing_3() {
        meta.rand = random_hash.get(hdr.record.key);
    }

    @stage(0)
    table set_port_table_0 {
        key = {
            ig_intr_md.ingress_port:exact;
        }
        actions = {
            set_port_action;
        }
        size = 16;
    }

    @stage(0)
    table set_port_table_1 {
        key = {
            ig_intr_md.ingress_port:exact;
        }
        actions = {
            set_port_action;
        }
        size = 16;
    }
    
    @stage(0)
    table preprocessing_table_1 {
        actions = {
            preprocessing_1;
        }
        size = 1;
        const default_action = preprocessing_1;
    }

    @stage(0)
    table preprocessing_table_2 {
        actions = {
            preprocessing_2;
        }
        size = 1;
        const default_action = preprocessing_2;
    }

    @stage(0)
    table preprocessing_table_3 {
        actions = {
            preprocessing_3;
        }
        size = 1;
        const default_action = preprocessing_3;
    }

/********************** LRU KEY *****************************/
/*--------------------- KEY 1 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) key_1;
    RegisterAction<b32_t, b32_t, b32_t>(key_1) read_key_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(key_1) replace_key_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.bridge.key;
        }
    };

    action read_key_1() {
        meta.key = read_key_salu_1.execute(hdr.bridge.index);
    }

    action replace_key_1() {
        meta.key = replace_key_salu_1.execute(hdr.bridge.index);  
    }

    @stage(2)
    table read_key_table_1 {
        actions = {
            read_key_1;
        }
        size = 1;
        const default_action = read_key_1;
    }

    @stage(2)
    table replace_key_table_1 {
        actions = {
            replace_key_1;
        }
        size = 1;
        const default_action = replace_key_1;
    }

/*--------------------- KEY 2 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) key_2;
    RegisterAction<b32_t, b32_t, b32_t>(key_2) read_key_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(key_2) replace_key_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.bridge.key;
        }
    };

    action read_key_2() {
        meta.key = read_key_salu_2.execute(hdr.bridge.index);
    }

    action replace_key_2() {
        meta.key = replace_key_salu_2.execute(hdr.bridge.index);  
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
    table replace_key_table_2 {
        actions = {
            replace_key_2;
        }
        size = 1;
        const default_action = replace_key_2;
    }

/*--------------------- KEY 3 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) key_3;
    RegisterAction<b32_t, b32_t, b32_t>(key_3) read_key_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(key_3) replace_key_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.bridge.key;
        }
    };

    action read_key_3() {
        meta.key = read_key_salu_3.execute(hdr.bridge.index);
    }

    action replace_key_3() {
        meta.key = replace_key_salu_3.execute(hdr.bridge.index);  
    }

    @stage(4)
    table read_key_table_3 {
        actions = {
            read_key_3;
        }
        size = 1;
        const default_action = read_key_3;
    }

    @stage(4)
    table replace_key_table_3 {
        actions = {
            replace_key_3;
        }
        size = 1;
        const default_action = replace_key_3;
    }

/********************** LRU ADDR *****************************/
/*--------------------- ADDR 1 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) addr_1_1;
    RegisterAction<b32_t, b32_t, b32_t>(addr_1_1) read_addr_1_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(addr_1_1) replace_addr_1_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_1;
        }
    };

    action read_addr_1_action_1() {
        hdr.record.addr_1 = read_addr_1_salu_1.execute(hdr.bridge.index);
    }

    action replace_addr_1_action_1() {
        hdr.bridge.addr_1 = replace_addr_1_salu_1.execute(hdr.bridge.index);
    }

    @stage(6) 
    table read_addr_1_table_1 {
        actions = {
            read_addr_1_action_1;
        }
        size = 1;
        const default_action = read_addr_1_action_1;
    }

    @stage(6) 
    table replace_addr_1_table_1 {
        actions = {
            replace_addr_1_action_1;
        }
        size = 1;
        const default_action = replace_addr_1_action_1;
    }

    Register<b16_t, b32_t>(BUCKET_SIZE) addr_2_1;
    RegisterAction<b16_t, b32_t, b16_t>(addr_2_1) read_addr_2_salu_1 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b16_t, b32_t, b16_t>(addr_2_1) replace_addr_2_salu_1 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_2;
        }
    };

    action read_addr_2_action_1() {
        hdr.record.addr_2 = read_addr_2_salu_1.execute(hdr.bridge.index);
    }

    action replace_addr_2_action_1() {
        hdr.bridge.addr_2 = replace_addr_2_salu_1.execute(hdr.bridge.index);
    }

    @stage(6) 
    table read_addr_2_table_1 {
        actions = {
            read_addr_2_action_1;
        }
        size = 1;
        const default_action = read_addr_2_action_1;
    }

    @stage(6) 
    table replace_addr_2_table_1 {
        actions = {
            replace_addr_2_action_1;
        }
        size = 1;
        const default_action = replace_addr_2_action_1;
    }

/*--------------------- ADDR 2 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) addr_1_2;
    RegisterAction<b32_t, b32_t, b32_t>(addr_1_2) read_addr_1_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(addr_1_2) replace_addr_1_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_1;
        }
    };

    action read_addr_1_action_2() {
        hdr.record.addr_1 = read_addr_1_salu_2.execute(hdr.bridge.index);
    }

    action replace_addr_1_action_2() {
        hdr.bridge.addr_1 = replace_addr_1_salu_2.execute(hdr.bridge.index);
    }

    @stage(7) 
    table read_addr_1_table_2 {
        actions = {
            read_addr_1_action_2;
        }
        size = 1;
        const default_action = read_addr_1_action_2;
    }

    @stage(7) 
    table replace_addr_1_table_2 {
        actions = {
            replace_addr_1_action_2;
        }
        size = 1;
        const default_action = replace_addr_1_action_2;
    }

    Register<b16_t, b32_t>(BUCKET_SIZE) addr_2_2;
    RegisterAction<b16_t, b32_t, b16_t>(addr_2_2) read_addr_2_salu_2 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b16_t, b32_t, b16_t>(addr_2_2) replace_addr_2_salu_2 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_2;
        }
    };

    action read_addr_2_action_2() {
        hdr.record.addr_2 = read_addr_2_salu_2.execute(hdr.bridge.index);
    }

    action replace_addr_2_action_2() {
        hdr.bridge.addr_2 = replace_addr_2_salu_2.execute(hdr.bridge.index);
    }

    @stage(7) 
    table read_addr_2_table_2 {
        actions = {
            read_addr_2_action_2;
        }
        size = 1;
        const default_action = read_addr_2_action_2;
    }

    @stage(7) 
    table replace_addr_2_table_2 {
        actions = {
            replace_addr_2_action_2;
        }
        size = 1;
        const default_action = replace_addr_2_action_2;
    }

/*--------------------- ADDR 3 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) addr_1_3;
    RegisterAction<b32_t, b32_t, b32_t>(addr_1_3) read_addr_1_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(addr_1_3) replace_addr_1_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_1;
        }
    };

    action read_addr_1_action_3() {
        hdr.record.addr_1 = read_addr_1_salu_3.execute(hdr.bridge.index);
    }

    action replace_addr_1_action_3() {
        hdr.bridge.addr_1 = replace_addr_1_salu_3.execute(hdr.bridge.index);
    }

    @stage(8) 
    table read_addr_1_table_3 {
        actions = {
            read_addr_1_action_3;
        }
        size = 1;
        const default_action = read_addr_1_action_3;
    }

    @stage(8) 
    table replace_addr_1_table_3 {
        actions = {
            replace_addr_1_action_3;
        }
        size = 1;
        const default_action = replace_addr_1_action_3;
    }

    Register<b16_t, b32_t>(BUCKET_SIZE) addr_2_3;
    RegisterAction<b16_t, b32_t, b16_t>(addr_2_3) read_addr_2_salu_3 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b16_t, b32_t, b16_t>(addr_2_3) replace_addr_2_salu_3 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_2;
        }
    };

    action read_addr_2_action_3() {
        hdr.record.addr_2 = read_addr_2_salu_3.execute(hdr.bridge.index);
    }

    action replace_addr_2_action_3() {
        hdr.bridge.addr_2 = replace_addr_2_salu_3.execute(hdr.bridge.index);
    }

    @stage(8) 
    table read_addr_2_table_3 {
        actions = {
            read_addr_2_action_3;
        }
        size = 1;
        const default_action = read_addr_2_action_3;
    }

    @stage(8) 
    table replace_addr_2_table_3 {
        actions = {
            replace_addr_2_action_3;
        }
        size = 1;
        const default_action = replace_addr_2_action_3;
    }
/********************** INGRESS LOGIC *****************************/	
	apply {
        //use a hash number(stored in hdr.bridge.flag) to decide which registers to be used
        //
        if(hdr.udp.isValid()){ 
            set_port_table_0.apply();
            //hash 
            preprocessing_table_1.apply();
            preprocessing_table_2.apply();  
            preprocessing_table_3.apply(); 
            
            //set hdr.bridge.flag
            if(meta.rand <= 682) {
                hdr.bridge.flag = 1;
            }
            else if(meta.rand <= 1364) {
                hdr.bridge.flag = 2;
            }
            else if(meta.rand <= 2046) {
                hdr.bridge.flag = 3;
            }
            else if(meta.rand <= 2728) {
                hdr.bridge.flag = 4;
            }
            else if(meta.rand <= 3410) {
                hdr.bridge.flag = 5;
            }
            else {
                hdr.bridge.flag = 6;
            }


            if(hdr.record.type == 0x0527) {     //read
                if(hdr.bridge.flag == 1) {
                    read_key_table_1.apply();
                }
                else if(hdr.bridge.flag == 2) {
                    read_key_table_2.apply();
                }
                else if(hdr.bridge.flag == 3) {
                    read_key_table_3.apply();
                }

                if(meta.key == hdr.bridge.key) {
                    if(hdr.bridge.flag == 1) {
                        read_addr_1_table_1.apply();
                        read_addr_2_table_1.apply();
                    }
                    else if(hdr.bridge.flag == 2) {
                        read_addr_1_table_2.apply();
                        read_addr_2_table_2.apply();
                    }
                    else if(hdr.bridge.flag == 3) {
                        read_addr_1_table_3.apply();
                        read_addr_2_table_3.apply();
                    }
                }
            }
            else if(hdr.record.type == 0x0529) {     //insert mode
                if(hdr.bridge.flag == 1) {
                    replace_key_table_1.apply();
                }
                else if(hdr.bridge.flag == 2) {
                    replace_key_table_2.apply();
                }
                else if(hdr.bridge.flag == 3) {
                    replace_key_table_3.apply();
                }

                if(hdr.bridge.flag == 1) {
                    replace_addr_1_table_1.apply();
                    replace_addr_2_table_1.apply();
                }
                else if(hdr.bridge.flag == 2) {
                    replace_addr_1_table_2.apply();
                    replace_addr_2_table_2.apply();
                }
                else if(hdr.bridge.flag == 3) {
                    replace_addr_1_table_3.apply();
                    replace_addr_2_table_3.apply();
                }
            }
        }
        else {
            // send other packets directly
            set_port_table_1.apply();
        }

	}
}


//implement the second part cache
control Ingress2(inout ingress_header_t hdr,
		inout ingress_metadata_t meta,
		in ingress_intrinsic_metadata_t ig_intr_md,
		in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
		inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
		inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
{

/********************** PREPROCESSING *****************************/

    action set_port_action(PortId_t port) {
        ig_tm_md.ucast_egress_port = port;
    }

    @stage(0)
    table set_port_table {
        key = {
            ig_intr_md.ingress_port:exact;
        }
        actions = {
            set_port_action;
        }
        size = 16;
    }

/********************** LRU KEY *****************************/
/*--------------------- KEY 1 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) key_1;
    RegisterAction<b32_t, b32_t, b32_t>(key_1) read_key_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(key_1) replace_key_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.bridge.key;
        }
    };

    action read_key_1() {
        meta.key = read_key_salu_1.execute(hdr.bridge.index);
    }

    action replace_key_1() {
        meta.key = replace_key_salu_1.execute(hdr.bridge.index);  
    }

    @stage(2)
    table read_key_table_1 {
        actions = {
            read_key_1;
        }
        size = 1;
        const default_action = read_key_1;
    }

    @stage(2)
    table replace_key_table_1 {
        actions = {
            replace_key_1;
        }
        size = 1;
        const default_action = replace_key_1;
    }

/*--------------------- KEY 2 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) key_2;
    RegisterAction<b32_t, b32_t, b32_t>(key_2) read_key_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(key_2) replace_key_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.bridge.key;
        }
    };

    action read_key_2() {
        meta.key = read_key_salu_2.execute(hdr.bridge.index);
    }

    action replace_key_2() {
        meta.key = replace_key_salu_2.execute(hdr.bridge.index);  
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
    table replace_key_table_2 {
        actions = {
            replace_key_2;
        }
        size = 1;
        const default_action = replace_key_2;
    }

/*--------------------- KEY 3 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) key_3;
    RegisterAction<b32_t, b32_t, b32_t>(key_3) read_key_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(key_3) replace_key_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.bridge.key;
        }
    };

    action read_key_3() {
        meta.key = read_key_salu_3.execute(hdr.bridge.index);
    }

    action replace_key_3() {
        meta.key = replace_key_salu_3.execute(hdr.bridge.index);  
    }

    @stage(4)
    table read_key_table_3 {
        actions = {
            read_key_3;
        }
        size = 1;
        const default_action = read_key_3;
    }

    @stage(4)
    table replace_key_table_3 {
        actions = {
            replace_key_3;
        }
        size = 1;
        const default_action = replace_key_3;
    }

/********************** LRU ADDR *****************************/
/*--------------------- ADDR 1 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) addr_1_1;
    RegisterAction<b32_t, b32_t, b32_t>(addr_1_1) read_addr_1_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(addr_1_1) replace_addr_1_salu_1 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_1;
        }
    };

    action read_addr_1_action_1() {
        hdr.record.addr_1 = read_addr_1_salu_1.execute(hdr.bridge.index);
    }

    action replace_addr_1_action_1() {
        hdr.bridge.addr_1 = replace_addr_1_salu_1.execute(hdr.bridge.index);
    }

    @stage(6) 
    table read_addr_1_table_1 {
        actions = {
            read_addr_1_action_1;
        }
        size = 1;
        const default_action = read_addr_1_action_1;
    }

    @stage(6) 
    table replace_addr_1_table_1 {
        actions = {
            replace_addr_1_action_1;
        }
        size = 1;
        const default_action = replace_addr_1_action_1;
    }

    Register<b16_t, b32_t>(BUCKET_SIZE) addr_2_1;
    RegisterAction<b16_t, b32_t, b16_t>(addr_2_1) read_addr_2_salu_1 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b16_t, b32_t, b16_t>(addr_2_1) replace_addr_2_salu_1 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_2;
        }
    };

    action read_addr_2_action_1() {
        hdr.record.addr_2 = read_addr_2_salu_1.execute(hdr.bridge.index);
    }

    action replace_addr_2_action_1() {
        hdr.bridge.addr_2 = replace_addr_2_salu_1.execute(hdr.bridge.index);
    }

    @stage(6) 
    table read_addr_2_table_1 {
        actions = {
            read_addr_2_action_1;
        }
        size = 1;
        const default_action = read_addr_2_action_1;
    }

    @stage(6) 
    table replace_addr_2_table_1 {
        actions = {
            replace_addr_2_action_1;
        }
        size = 1;
        const default_action = replace_addr_2_action_1;
    }

/*--------------------- ADDR 2 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) addr_1_2;
    RegisterAction<b32_t, b32_t, b32_t>(addr_1_2) read_addr_1_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(addr_1_2) replace_addr_1_salu_2 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_1;
        }
    };

    action read_addr_1_action_2() {
        hdr.record.addr_1 = read_addr_1_salu_2.execute(hdr.bridge.index);
    }

    action replace_addr_1_action_2() {
        hdr.bridge.addr_1 = replace_addr_1_salu_2.execute(hdr.bridge.index);
    }

    @stage(7) 
    table read_addr_1_table_2 {
        actions = {
            read_addr_1_action_2;
        }
        size = 1;
        const default_action = read_addr_1_action_2;
    }

    @stage(7) 
    table replace_addr_1_table_2 {
        actions = {
            replace_addr_1_action_2;
        }
        size = 1;
        const default_action = replace_addr_1_action_2;
    }

    Register<b16_t, b32_t>(BUCKET_SIZE) addr_2_2;
    RegisterAction<b16_t, b32_t, b16_t>(addr_2_2) read_addr_2_salu_2 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b16_t, b32_t, b16_t>(addr_2_2) replace_addr_2_salu_2 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_2;
        }
    };

    action read_addr_2_action_2() {
        hdr.record.addr_2 = read_addr_2_salu_2.execute(hdr.bridge.index);
    }

    action replace_addr_2_action_2() {
        hdr.bridge.addr_2 = replace_addr_2_salu_2.execute(hdr.bridge.index);
    }

    @stage(7) 
    table read_addr_2_table_2 {
        actions = {
            read_addr_2_action_2;
        }
        size = 1;
        const default_action = read_addr_2_action_2;
    }

    @stage(7) 
    table replace_addr_2_table_2 {
        actions = {
            replace_addr_2_action_2;
        }
        size = 1;
        const default_action = replace_addr_2_action_2;
    }

/*--------------------- ADDR 3 ---------------------------*/
    Register<b32_t, b32_t>(BUCKET_SIZE) addr_1_3;
    RegisterAction<b32_t, b32_t, b32_t>(addr_1_3) read_addr_1_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b32_t, b32_t, b32_t>(addr_1_3) replace_addr_1_salu_3 = {
        void apply (inout b32_t reg_data, out b32_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_1;
        }
    };

    action read_addr_1_action_3() {
        hdr.record.addr_1 = read_addr_1_salu_3.execute(hdr.bridge.index);
    }

    action replace_addr_1_action_3() {
        hdr.bridge.addr_1 = replace_addr_1_salu_3.execute(hdr.bridge.index);
    }

    @stage(8) 
    table read_addr_1_table_3 {
        actions = {
            read_addr_1_action_3;
        }
        size = 1;
        const default_action = read_addr_1_action_3;
    }

    @stage(8) 
    table replace_addr_1_table_3 {
        actions = {
            replace_addr_1_action_3;
        }
        size = 1;
        const default_action = replace_addr_1_action_3;
    }

    Register<b16_t, b32_t>(BUCKET_SIZE) addr_2_3;
    RegisterAction<b16_t, b32_t, b16_t>(addr_2_3) read_addr_2_salu_3 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
        }
    };

    RegisterAction<b16_t, b32_t, b16_t>(addr_2_3) replace_addr_2_salu_3 = {
        void apply (inout b16_t reg_data, out b16_t out_data) {
            out_data = reg_data;
            reg_data = hdr.record.addr_2;
        }
    };

    action read_addr_2_action_3() {
        hdr.record.addr_2 = read_addr_2_salu_3.execute(hdr.bridge.index);
    }

    action replace_addr_2_action_3() {
        hdr.bridge.addr_2 = replace_addr_2_salu_3.execute(hdr.bridge.index);
    }

    @stage(8) 
    table read_addr_2_table_3 {
        actions = {
            read_addr_2_action_3;
        }
        size = 1;
        const default_action = read_addr_2_action_3;
    }

    @stage(8) 
    table replace_addr_2_table_3 {
        actions = {
            replace_addr_2_action_3;
        }
        size = 1;
        const default_action = replace_addr_2_action_3;
    }

/********************** INGRESS LOGIC *****************************/	
	apply {
        set_port_table.apply();
        if(hdr.record.type == 0x0527) {
            if(hdr.bridge.flag == 4) {
                read_key_table_1.apply();
            }
            else if(hdr.bridge.flag == 5) {
                read_key_table_2.apply();
            }
            else if(hdr.bridge.flag == 6) {
                read_key_table_3.apply();
            }

            if(meta.key == hdr.bridge.key) {
                if(hdr.bridge.flag == 4) {
                    read_addr_1_table_1.apply();
                    read_addr_2_table_1.apply();
                }
                else if(hdr.bridge.flag == 5) {
                    read_addr_1_table_2.apply();
                    read_addr_2_table_2.apply();
                }
                else if(hdr.bridge.flag == 6) {
                    read_addr_1_table_3.apply();
                    read_addr_2_table_3.apply();
                }
            }
        }
        else if(hdr.record.type == 0x0529) {
            if(hdr.bridge.flag == 4) {
                replace_key_table_1.apply();
            }
            else if(hdr.bridge.flag == 5) {
                replace_key_table_2.apply();
            }
            else if(hdr.bridge.flag == 6) {
                replace_key_table_3.apply();
            }

            if(hdr.bridge.flag == 4) {
                replace_addr_1_table_1.apply();
                replace_addr_2_table_1.apply();
            }
            else if(hdr.bridge.flag == 5) {
                replace_addr_1_table_2.apply();
                replace_addr_2_table_2.apply();
            }
            else if(hdr.bridge.flag == 6) {
                replace_addr_1_table_3.apply();
                replace_addr_2_table_3.apply();
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
parser EgressParser1(packet_in pkt,
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
        pkt.extract(hdr.bridge);
		transition accept;
	}
}

parser EgressParser2(packet_in pkt,
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
        pkt.extract(hdr.bridge);
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
        if(hdr.udp.isValid()) {
            hdr.bridge.setInvalid();
        }
    }
}

control Egress2(inout egress_header_t hdr,
	inout egress_metadata_t meta,
	in egress_intrinsic_metadata_t eg_intr_md,
	in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
	inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
	inout egress_intrinsic_metadata_for_output_port_t eg_oport_md)
{
	apply{}
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
/* LRU-1*/
Pipeline(IngressParser1(),Ingress1(),IngressDeparser(),
EgressParser1(),Egress1(),EgressDeparser()) pipe1;

/*LRU-2*/
Pipeline(IngressParser2(),Ingress2(),IngressDeparser(),
EgressParser2(),Egress2(),EgressDeparser()) pipe2;

Switch(pipe1,pipe2) main;

