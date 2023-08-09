/*
* 2022-5-24
* four stage filters and four stage hash tables
* eight exact match tables in filter stages, for higher utilization of SRAM
* two-layer sketch filter
* add ring buffer, put hash table to egress of pipe2
* 5-24, update buffer index
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
#define __TEST__
const bit<32> FILTER_SIZE = 1<<18;	//18-bit 
const bit<32> BUCKET_SIZE = 1<<10;	//17-bit
const bit<32> BUFFER_POOL_SIZE = 1<<11;     //
const bit<8> PACKET_BUFFER_SIZE = 1<<7;     //128 (fp.counter) tuple records per submit packet
typedef bit<19> filter_hash_size_t;
typedef bit<17> bucket_hash_size_t;
typedef bit<32> lru_fp_length_t;
typedef bit<32> lru_counter_length_t;
typedef bit<32> filter_size_t;
typedef bit<32> bucket_size_t;
typedef bit<16> filter_counter_length_t;	
typedef bit<16> filter_timestamp_length_t;
typedef bit<16> buffer_index_length_t;
typedef bit<16> buffer_size_t;
typedef bit<8> pair_size_length_t;
typedef bit<1> bool_size_t;

struct hash_bucket_t {
	bit<32> lo;
	bit<32> hi;
}

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

header filter_record_h {
	bit<32> index_1;
	bit<32> index_2;
	bit<16> timestamp;
	bit<1> filter_is_replace_1;
	bit<1> filter_is_replace_2;
	bit<6> zero_1;
}

header lru_record_h {
	bit<32> fp;
	bit<32> counter;
	bit<32> old_fp;
	bit<32> old_counter;
	bit<32> index;
	bit<1> lru_is_replace;
    bit<1> is_submit;
	bit<6> zero;
}

header buffer_record_h {
    bit<16> begin;
    bit<16> end;
    bit<16> pointer;    //set pointer = begin, then inc pointer; if pointer == end, end recirculate
}

header pair_record_h {
    bit<32> fp;
    bit<32> counter;
}

header submit_record_t {
	bit<32> src_addr;
	bit<32> dst_addr;
	bit<16> src_port;
	bit<16> dst_port;
	bit<8> protocol;
    bit<32> counter;
	bit<32> old_fp;
	bit<32> new_fp;
}

struct ingress_metadata_t {
	bit<16> filter_counter_1;
	bit<16> filter_counter_2;
	bit<16> dif_counter_12;
}

struct ingress_header_t {
	ethernet_h ethernet;
	ipv4_h ipv4;
	udp_h udp;
	submit_record_t submit_record;
	filter_record_h filter_record;
	lru_record_h lru_record;
}

struct egress_header_t {
	ethernet_h ethernet;
	ipv4_h ipv4;
	udp_h udp;
	submit_record_t submit_record;
	filter_record_h filter_record;
	lru_record_h lru_record;
}

struct egress_metadata_t {
    bit<32> key_evt_1;
    bit<32> key_evt_2;
    bit<32> key_evt_3;

    bit<8> dfa_tsm;
    bit<8> dfa_sta;
}

/*************************************************************************
 **************I N G R E S S  P A R S E R  *******************************
*************************************************************************/
parser IngressParser1(
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
		transition parse_test;
	}
	
	state parse_test {
		pkt.extract(hdr.submit_record);
		hdr.filter_record.setValid();
		hdr.lru_record.setValid();
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
		transition parse_test;
	}
	
	state parse_test {
		pkt.extract(hdr.submit_record);
		pkt.extract(hdr.filter_record);
		pkt.extract(hdr.lru_record);  
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
/**************************** PREPROCESSING ********************************/
    CRCPolynomial<bit<32>>(coeff=0x04C11DB7,reversed=true, msb=false, extended=false, init=0xFFFFFFFF, xor=0xFFFFFFFF) crc32_1;
	Hash<filter_hash_size_t>(HashAlgorithm_t.CUSTOM, crc32_1) filter_hash_1;
	Hash<filter_hash_size_t>(HashAlgorithm_t.CRC32) filter_hash_2;
	action filter_preprocessing_0() {
		hdr.filter_record.index_1 =  (filter_size_t)filter_hash_1.get({hdr.submit_record.src_addr,hdr.submit_record.dst_addr,hdr.submit_record.src_port,hdr.submit_record.dst_port,hdr.submit_record.protocol});
		hdr.filter_record.timestamp = ig_intr_md.ingress_mac_tstamp[39:24];
	}
	
	@stage(0)
	table filter_preprocessing_table_0 {
		actions = {
			filter_preprocessing_0;
		}
		size = 1;
		const default_action = filter_preprocessing_0;
	}

	action filter_preprocessing_1() {
		hdr.filter_record.index_2 = (filter_size_t)filter_hash_2.get({hdr.submit_record.src_addr,hdr.submit_record.dst_addr,hdr.submit_record.src_port,hdr.submit_record.dst_port,hdr.submit_record.protocol});
	}
	
	@stage(0)
	table filter_preprocessing_table_1 {
		actions = {
			filter_preprocessing_1;
		}
		size = 1;
		const default_action = filter_preprocessing_1;
	}

/*********************** FILTER TIMESTAMP CHECK *****************************/
/***************************** STAGE 1 **************************************/
	Register<filter_timestamp_length_t,filter_size_t>(FILTER_SIZE) filter_timestamp_buckets_00;
	RegisterAction<filter_timestamp_length_t,filter_size_t,bool_size_t>(filter_timestamp_buckets_00) filter_timestamp_buckets_salu_00 = {
		void apply(inout filter_timestamp_length_t reg_data, out bool_size_t is_replace) {
			if(hdr.filter_record.timestamp == reg_data) {
				is_replace = 0;
			}
			else {
				reg_data = hdr.filter_record.timestamp;
				is_replace = 1;
			}
		}
	};
	action filter_timestamp_check_00() {
		hdr.filter_record.filter_is_replace_1 = filter_timestamp_buckets_salu_00.execute((filter_size_t)hdr.filter_record.index_1[17:0]);
	}

	@stage(1)
	table filter_timestamp_check_table_00 {
		actions = {
			filter_timestamp_check_00;
		}
		size = 1;
		const default_action = filter_timestamp_check_00;
	}
	
/***************************** STAGE 2 **************************************/
	Register<filter_timestamp_length_t,filter_size_t>(FILTER_SIZE) filter_timestamp_buckets_01;
	RegisterAction<filter_timestamp_length_t,filter_size_t,bool_size_t>(filter_timestamp_buckets_01) filter_timestamp_buckets_salu_01 = {
		void apply(inout filter_timestamp_length_t reg_data, out bool_size_t is_replace) {
			if(hdr.filter_record.timestamp == reg_data) {
				is_replace = 0;
			}
			else {
				reg_data = hdr.filter_record.timestamp;
				is_replace = 1;
			}
		}
	};
	action filter_timestamp_check_01() {
		hdr.filter_record.filter_is_replace_1 = filter_timestamp_buckets_salu_01.execute((filter_size_t)hdr.filter_record.index_1[17:0]);
	}

	@stage(2)
	table filter_timestamp_check_table_01 {
		actions = {
			filter_timestamp_check_01;
		}
		size = 1;
		const default_action = filter_timestamp_check_01;
	}

/***************************** STAGE 3 **************************************/
	Register<filter_timestamp_length_t,filter_size_t>(FILTER_SIZE) filter_timestamp_buckets_10;
	RegisterAction<filter_timestamp_length_t,filter_size_t,bool_size_t>(filter_timestamp_buckets_10) filter_timestamp_buckets_salu_10 = {
		void apply(inout filter_timestamp_length_t reg_data, out bool_size_t is_replace) {
			if(hdr.filter_record.timestamp == reg_data) {
				is_replace = 0;
			}
			else {
				reg_data = hdr.filter_record.timestamp;
				is_replace = 1;
			}
		}
	};
	action filter_timestamp_check_10() {
		hdr.filter_record.filter_is_replace_2 = filter_timestamp_buckets_salu_10.execute((filter_size_t)hdr.filter_record.index_2[17:0]);
	}
	
	@stage(3)
	table filter_timestamp_check_table_10 {
		actions = {
			filter_timestamp_check_10;
		}
		size = 1;
		const default_action = filter_timestamp_check_10;
	}

/***************************** STAGE 4 **************************************/
	Register<filter_timestamp_length_t,filter_size_t>(FILTER_SIZE) filter_timestamp_buckets_11;
	RegisterAction<filter_timestamp_length_t,filter_size_t,bool_size_t>(filter_timestamp_buckets_11) filter_timestamp_buckets_salu_11 = {
		void apply(inout filter_timestamp_length_t reg_data, out bool_size_t is_replace) {
			if(hdr.filter_record.timestamp == reg_data) {
				is_replace = 0;
			}
			else {
				reg_data = hdr.filter_record.timestamp;
				is_replace = 1;
			}
		}
	};
	action filter_timestamp_check_11() {
		hdr.filter_record.filter_is_replace_2 = filter_timestamp_buckets_salu_11.execute((filter_size_t)hdr.filter_record.index_2[17:0]);
	}
	
	@stage(4)
	table filter_timestamp_check_table_11 {
		actions = {
			filter_timestamp_check_11;
		}
		size = 1;
		const default_action = filter_timestamp_check_11;
	}

/********************** FILTER COUNTER UPDATE *****************************/
/***************************** STAGE 5 **************************************/
	Register<filter_counter_length_t,filter_size_t>(FILTER_SIZE) filter_counter_buckets_00;
	RegisterAction<filter_counter_length_t,filter_size_t,filter_counter_length_t>(filter_counter_buckets_00) filter_counter_buckets_salu_00 = {
		void apply(inout filter_counter_length_t reg_data, out filter_counter_length_t out_data) {
			if(hdr.filter_record.filter_is_replace_1 == 1) {
				reg_data = hdr.submit_record.counter[15:0];
				out_data = 0;
			}
			else {
				out_data = reg_data;
				reg_data = reg_data + hdr.submit_record.counter[15:0];
			}
		}
	};

	action filter_counter_update_00() {
		meta.filter_counter_1 = filter_counter_buckets_salu_00.execute((filter_size_t)hdr.filter_record.index_1[17:0]);
	}
	
	@stage(5)
	table filter_counter_update_table_00 {
		actions = {
			filter_counter_update_00;
		}
		size = 1;
		const default_action = filter_counter_update_00;
	}

/***************************** STAGE 6 **************************************/
	Register<filter_counter_length_t,filter_size_t>(FILTER_SIZE) filter_counter_buckets_01;
	RegisterAction<filter_counter_length_t,filter_size_t,filter_counter_length_t>(filter_counter_buckets_01) filter_counter_buckets_salu_01 = {
		void apply(inout filter_counter_length_t reg_data, out filter_counter_length_t out_data) {
			if(hdr.filter_record.filter_is_replace_1 == 1) {
				reg_data = hdr.submit_record.counter[15:0];
				out_data = 0;
			}
			else {
				out_data = reg_data;
				reg_data = reg_data + hdr.submit_record.counter[15:0];
			}
		}
	};

	action filter_counter_update_01() {
		meta.filter_counter_1 = filter_counter_buckets_salu_01.execute((filter_size_t)hdr.filter_record.index_1[17:0]);
	}
	
	@stage(6)
	table filter_counter_update_table_01 {
		actions = {
			filter_counter_update_01;
		}
		size = 1;
		const default_action = filter_counter_update_01;
	}

/***************************** STAGE 7 **************************************/
	Register<filter_counter_length_t,filter_size_t>(FILTER_SIZE) filter_counter_buckets_10;
	RegisterAction<filter_counter_length_t,filter_size_t,filter_counter_length_t>(filter_counter_buckets_10) filter_counter_buckets_salu_10 = {
		void apply(inout filter_counter_length_t reg_data, out filter_counter_length_t out_data) {
			if(hdr.filter_record.filter_is_replace_2 == 1) {
				reg_data = hdr.submit_record.counter[15:0];
				out_data = 0;
			}
			else {
				out_data = reg_data;
				reg_data = reg_data + hdr.submit_record.counter[15:0];
			}
		}
	};

	action filter_counter_update_10() {
		meta.filter_counter_2 = filter_counter_buckets_salu_10.execute((filter_size_t)hdr.filter_record.index_2[17:0]);
	}
	
	@stage(7)
	table filter_counter_update_table_10 {
		actions = {
			filter_counter_update_10;
		}
		size = 1;
		const default_action = filter_counter_update_10;
	}

/***************************** STAGE 8 **************************************/
	Register<filter_counter_length_t,filter_size_t>(FILTER_SIZE) filter_counter_buckets_11;
	RegisterAction<filter_counter_length_t,filter_size_t,filter_counter_length_t>(filter_counter_buckets_11) filter_counter_buckets_salu_11 = {
		void apply(inout filter_counter_length_t reg_data, out filter_counter_length_t out_data) {
			if(hdr.filter_record.filter_is_replace_2 == 1) {
				reg_data = hdr.submit_record.counter[15:0];
				out_data = 0;
			}
			else {
				out_data = reg_data;
				reg_data = reg_data + hdr.submit_record.counter[15:0];
			}
		}
	};

	action filter_counter_update_11() {
		meta.filter_counter_2 = filter_counter_buckets_salu_11.execute((filter_size_t)hdr.filter_record.index_2[17:0]);
	}
	
	@stage(8)
	table filter_counter_update_table_11 {
		actions = {
			filter_counter_update_11;
		}
		size = 1;
		const default_action = filter_counter_update_11;
	}

/********************** find min *****************************/
	Register<filter_counter_length_t, bit<1>>(1) find_min_counter;
	RegisterAction<filter_counter_length_t, bit<1>, filter_counter_length_t>(find_min_counter) find_min_counter_salu = {
		void apply(inout filter_counter_length_t reg_data, out filter_counter_length_t out_data) {
			if(meta.dif_counter_12[15:15] == 1) {
				out_data = 0;
			}
			else {
				out_data = 1;
			}
		}
	};

	action find_min_counter_12() {
		meta.dif_counter_12 = find_min_counter_salu.execute(0);
	}

	table find_min_counter_table {
		actions = {
			find_min_counter_12;
		}
		size = 1;
		const default_action = find_min_counter_12;
	}
/********************** THRESHOLD *****************************/
/***************************** STAGE 11 **************************************/
	Register<filter_counter_length_t, bit<1>>(1) counter_threshold;
	RegisterAction<filter_counter_length_t, bit<1>, bit<3>>(counter_threshold) counter_threshold_salu_1 = {
		void apply(inout filter_counter_length_t threshold, out bit<3> drop_ctl) {
			if(meta.filter_counter_1 >= threshold) {
				drop_ctl = 0;
			}
			else {
				drop_ctl = 1;
			}
		}
	};

	action counter_threshold_check_1() {
		ig_dprsr_md.drop_ctl = counter_threshold_salu_1.execute(0);
	}
	
	@stage(11)
	table counter_threshold_check_table_1 {
		actions = {
			counter_threshold_check_1;
		}
		size = 1;
		const default_action = counter_threshold_check_1;
	}

	RegisterAction<filter_counter_length_t, bit<1>, bit<3>>(counter_threshold) counter_threshold_salu_2 = {
		void apply(inout filter_counter_length_t threshold, out bit<3> drop_ctl) {
			if(meta.filter_counter_2 >= threshold) {
				drop_ctl = 0;
			}
			else {
				drop_ctl = 1;
			}
		}
	};

	action counter_threshold_check_2() {
		ig_dprsr_md.drop_ctl = counter_threshold_salu_2.execute(0);
	}
	
	@stage(11)
	table counter_threshold_check_table_2 {
		actions = {
			counter_threshold_check_2;
		}
		size = 1;
		const default_action = counter_threshold_check_2;
	}


/********************** FORWARD LOGIC *****************************/
	action simple_send(PortId_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	table simple_send_table_to_re {
		key = {
			ig_intr_md.ingress_port: exact;	
		}
		actions = {
			simple_send;
		}
		size = 32;
	}

	table simple_send_table {
		key = {
			ig_intr_md.ingress_port: exact;	
		}
		actions = {
			simple_send;
		}
		size = 32;
	}

	/*
	action drop() {
		ig_dprsr_md.drop_ctl = 1;
	}

	table drop_table {
		actions = {
			drop;
		}
		size = 1;
		const default_action = drop;
	}
	*/

/********************** INGRESS LOGIC *****************************/	
	apply {
		if(hdr.udp.isValid()) {
			filter_preprocessing_table_0.apply();
			filter_preprocessing_table_1.apply();
			//stage 1, layer 1
			if(hdr.filter_record.index_1[18:18] == 0) {
				filter_timestamp_check_table_00.apply();
			}
			//stage 2, layer 1
			if(hdr.filter_record.index_1[18:18] == 1) {
				filter_timestamp_check_table_01.apply();
			}
			//stage 3, layer 2
			if(hdr.filter_record.index_2[18:18] == 0) {
				filter_timestamp_check_table_10.apply();
			}
			//stage 4, layer 2
			if(hdr.filter_record.index_2[18:18] == 1) {
				filter_timestamp_check_table_11.apply();
			}
			//stage 5, layer 1
			if(hdr.filter_record.index_1[18:18] == 0) {
				filter_counter_update_table_00.apply();
			}
			//stage 6, layer 1
			if(hdr.filter_record.index_1[18:18] == 1) {
				filter_counter_update_table_01.apply();
			}
			//stage 7, layer 2
			if(hdr.filter_record.index_2[18:18] == 0) {
				filter_counter_update_table_10.apply();
			}
			//stage 8, layer 2
			if(hdr.filter_record.index_2[18:18] == 1) {
				filter_counter_update_table_11.apply();
			}
			//stage 9-10, find min counter
			meta.dif_counter_12 = meta.filter_counter_1 - meta.filter_counter_2;
			find_min_counter_table.apply();
			//stage	11, threshold
			if(meta.dif_counter_12 == 0) {
				counter_threshold_check_table_1.apply();
			}
			else {
				counter_threshold_check_table_2.apply();
			}
			simple_send_table_to_re.apply();
		}	
		else {
			simple_send_table.apply();
		}
	}
}


control Ingress2(inout ingress_header_t hdr,
		inout ingress_metadata_t meta,
		in ingress_intrinsic_metadata_t ig_intr_md,
		in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
		inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
		inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
{
// /********************** BUFFER POOL INDEX *****************************/
//     Register<buffer_index_length_t, bit<1>>(1) begin_index_reg;
//     Register<buffer_index_length_t, bit<1>>(1) end_index_reg;
//     RegisterAction<buffer_index_length_t, bit<1>, buffer_index_length_t>(begin_index_reg) read_begin_salu = {
//         void apply(inout buffer_index_length_t reg_data, out buffer_index_length_t out_data) {
//             out_data = reg_data;
//         }
//     };

//     RegisterAction<buffer_index_length_t, bit<2>, buffer_index_length_t>(begin_index_reg) write_begin_salu = {
//         void apply(inout buffer_index_length_t reg_data, out buffer_index_length_t out_data) {
//             out_data = 0;
// 			if(reg_data == 1920) {
// 				reg_data = 0;
// 			}
// 			else {
// 				reg_data = reg_data + 128;
// 			}
//         }
//     };
    
//     RegisterAction<buffer_index_length_t, bit<1>, buffer_index_length_t>(end_index_reg) inc_end_salu = {
//         void apply(inout buffer_index_length_t reg_data, out buffer_index_length_t out_data) {
//             if(reg_data == 2047) {
// 				reg_data = 0;
// 			}
// 			else {
// 				reg_data = reg_data+1;
// 			}
//             out_data = reg_data;
//         }
//     };

//     action read_begin() {
//         hdr.buffer_record.begin = read_begin_salu.execute(0);
//     }
    
//     @stage(1)
//     table read_begin_table {
//         actions = {
//             read_begin;
//         }
//         size = 1;
//         const default_action = read_begin;
//     }

//     action inc_end() {
//         hdr.buffer_record.end = inc_end_salu.execute(0);
//     }

//     @stage(2)
//     table inc_end_table {
//         actions = {
//             inc_end;
//         }
//         size = 1;
//         const default_action = inc_end;
//     }

//     action write_begin() {
//         write_begin_salu.execute(0);
//     }

//     @stage(1)
//     table write_begin_table {
//         actions = {
//             write_begin;
//         }
//         size = 1;
//         const default_action = write_begin;
//     }

// /********************** FP BUFFER POOL *****************************/
//     Register<lru_fp_length_t, buffer_size_t>(BUFFER_POOL_SIZE) fp_buffer;
//     RegisterAction<lru_fp_length_t, buffer_size_t, lru_fp_length_t>(fp_buffer) read_fp_buffer_salu = {
//         void apply(inout lru_fp_length_t reg_data, out lru_fp_length_t out_data) {
//             out_data = reg_data;
//         }
//     };

//     RegisterAction<lru_fp_length_t, buffer_size_t, lru_fp_length_t>(fp_buffer) write_fp_buffer_salu = {
//         void apply(inout lru_fp_length_t reg_data, out lru_fp_length_t out_data) {
//             out_data = reg_data;
//             reg_data = hdr.lru_record.old_fp;
//         }
//     };

//     action read_fp_buffer() {
//         hdr.pair_record.fp = read_fp_buffer_salu.execute(hdr.buffer_record.pointer);
//     }

//     @stage(4)
//     table read_fp_buffer_table {
//         actions = {
//             read_fp_buffer;
//         }
//         size = 1;
//         const default_action = read_fp_buffer;
//     }

//     action write_fp_buffer() {
//         write_fp_buffer_salu.execute(hdr.buffer_record.end);
//     }

//     @stage(4)
//     table write_fp_buffer_table {
//         actions = {
//             write_fp_buffer;
//         }
//         size = 1;
//         const default_action = write_fp_buffer;
//     }

// /********************** COUNTER BUFFER POOL *****************************/
//     Register<lru_counter_length_t, buffer_size_t>(BUFFER_POOL_SIZE) counter_buffer;
//     RegisterAction<lru_counter_length_t, buffer_size_t, lru_counter_length_t>(counter_buffer) read_counter_buffer_salu = {
//         void apply(inout lru_counter_length_t reg_data, out lru_counter_length_t out_data) {
//             out_data = reg_data;
//         }
//     };

//     RegisterAction<lru_counter_length_t, buffer_size_t, lru_counter_length_t>(counter_buffer) write_counter_buffer_salu = {
//         void apply(inout lru_counter_length_t reg_data, out lru_counter_length_t out_data) {
//             out_data = reg_data;
//             reg_data = hdr.lru_record.old_counter;
//         }
//     };

//     action read_counter_buffer() {
//         hdr.pair_record.counter = read_counter_buffer_salu.execute(hdr.buffer_record.pointer);
//     }

//     @stage(5)
//     table read_counter_buffer_table {
//         actions = {
//             read_counter_buffer;
//         }
//         size = 1;
//         const default_action = read_counter_buffer;
//     }

//     action write_counter_buffer() {
//         write_counter_buffer_salu.execute(hdr.buffer_record.end);
//     }

//     @stage(5)
//     table write_counter_buffer_table {
//         actions = {
//             write_counter_buffer;
//         }
//         size = 1;
//         const default_action = write_counter_buffer;
//     }

// /********************** FORWARD LOGIC *****************************/
	action simple_send_from_re(PortId_t port) {
		ig_tm_md.ucast_egress_port = port;
	}


	table simple_send_table_from_re {
		key = {
			ig_intr_md.ingress_port: exact;	
		}
		actions = {
			simple_send_from_re;
		}
		size = 32;
	}
	
// 	action resubmit_send() {
// 		ig_tm_md.ucast_egress_port = ig_intr_md.ingress_port;      //
// 	}

//     @stage(11)
// 	table resubmit_send_table {
// 		actions = {
// 			resubmit_send;
// 		}
// 		size = 1;
//         const default_action = resubmit_send;
// 	}

	action drop() {
		ig_dprsr_md.drop_ctl = 1;
	}

	table drop_table {
		actions = {
			drop;
		}
		size = 1;
		const default_action = drop;
	}

/********************** INGRESS LOGIC *****************************/	
	apply {
		// if(hdr.lru_record.is_submit == 1) {
        //     if(hdr.buffer_record.pointer == hdr.buffer_record.end) {    //end read
        //         write_begin_table.apply();
        //         simple_send_table_from_re.apply();
        //     }
        //     else {  //read one pair
        //         hdr.pair_record.setValid();
        //         hdr.pair_record.fp = 0;
        //         hdr.pair_record.counter = 0;
                
        //         read_fp_buffer_table.apply();
        //         read_counter_buffer_table.apply();
        //         hdr.buffer_record.pointer = hdr.buffer_record.pointer + 1;
		// 		if(hdr.buffer_record.pointer == 2048) {
		// 			hdr.buffer_record.pointer = 0;
		// 		}

        //         resubmit_send_table.apply();
        //     }
        // }
        // else {      //a normal lru_is_replace packet
        //     if(hdr.lru_record.lru_is_replace == 1) {
        //         //read begin and end
        //         read_begin_table.apply();
        //         inc_end_table.apply();

        //         //write info to buffer
        //         write_fp_buffer_table.apply();
        //         write_counter_buffer_table.apply();

        //         //submit event? 
        //         if(hdr.buffer_record.end[6:0] == 0) {        //submit
        //             hdr.lru_record.is_submit = 1;
		// 			if(hdr.buffer_record.end != 0) {
		// 				hdr.buffer_record.pointer = hdr.buffer_record.end-128;
		// 			}
		// 			else {
		// 				hdr.buffer_record.pointer = 1920;
		// 			}
        //             resubmit_send_table.apply();
        //         }
        //         else {
        //             drop_table.apply();
        //         }
        //     }
        //     else {
        //         drop_table.apply();
        //     }
        // }


		if(hdr.udp.isValid()) {
			if(hdr.lru_record.lru_is_replace == 1) {
				hdr.submit_record.old_fp = hdr.lru_record.old_fp;
				hdr.submit_record.counter = hdr.lru_record.old_counter;
				hdr.submit_record.new_fp = hdr.lru_record.fp;
				simple_send_table_from_re.apply();
			}
			else {
				drop_table.apply();
			}
			hdr.filter_record.setInvalid();
			hdr.lru_record.setInvalid();
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
		transition accept;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.ether_type) {
			0x0800: parse_ipv4;
			default: accept;
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
		transition parse_test;
	}

	
	state parse_test {
		pkt.extract(hdr.submit_record);
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
			default: accept;
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
		transition parse_test;
	}

	state parse_test {
		pkt.extract(hdr.submit_record);
		pkt.extract(hdr.filter_record);
		pkt.extract(hdr.lru_record);	
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

control Egress2(inout egress_header_t hdr,
	inout egress_metadata_t meta,
	in egress_intrinsic_metadata_t eg_intr_md,
	in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
	inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
	inout egress_intrinsic_metadata_for_output_port_t eg_oport_md)
{
/**************************** PREPROCESSING ********************************/
	CRCPolynomial<bit<32>>(coeff=0x1A833982B,reversed=false, msb=false, extended=false, init=0x00000000 , xor=0xFFFFFFFF) crc32_2;
	Hash<bucket_size_t>(HashAlgorithm_t.CRC32) lru_bucket_hash;
	Hash<lru_fp_length_t>(HashAlgorithm_t.CUSTOM,crc32_2) fp_hash;
	action lru_preprocessing() {
		hdr.lru_record.index = (bucket_size_t)lru_bucket_hash.get({hdr.submit_record.src_addr,hdr.submit_record.dst_addr,hdr.submit_record.src_port,hdr.submit_record.dst_port,hdr.submit_record.protocol});
		hdr.lru_record.counter = hdr.submit_record.counter;
	}

	@stage(0)
	table lru_preprocessing_table {
		actions = {
			lru_preprocessing;
		}
		size = 1;
		const default_action = lru_preprocessing;
	}

	action lru_preprocessing_1() {
		hdr.lru_record.fp = fp_hash.get({hdr.submit_record.src_addr,hdr.submit_record.dst_addr,hdr.submit_record.src_port,hdr.submit_record.dst_port,hdr.submit_record.protocol});
	}

	@stage(0)
	table lru_preprocessing_table_1 {
		actions = {
			lru_preprocessing_1;
		}
		size = 1;
		const default_action = lru_preprocessing_1;
	}
/**************************** LRU KEY ********************************/
/*********************** FP 1 **************************/
	Register<lru_fp_length_t, bucket_size_t>(BUCKET_SIZE) fp_1;
	RegisterAction<lru_fp_length_t, bucket_size_t, lru_fp_length_t>(fp_1) insert_fp_salu_1 = {
		void apply(inout lru_fp_length_t reg_data, out lru_fp_length_t out_data) {
			out_data = reg_data;
			reg_data = hdr.lru_record.fp;
		}
	};

	action insert_fp_action_1() {
		meta.key_evt_1 = insert_fp_salu_1.execute((bit<32>)hdr.lru_record.index[10:0]);
	}

	@stage(1)
	table insert_fp_table_1 {
		actions = {
			insert_fp_action_1;
		}
		size = 1;
		const default_action = insert_fp_action_1;
	}

/*********************** FP 2 **************************/
	Register<lru_fp_length_t, bucket_size_t>(BUCKET_SIZE) fp_2;
	RegisterAction<lru_fp_length_t, bucket_size_t, lru_fp_length_t>(fp_2) insert_fp_salu_2 = {
		void apply(inout lru_fp_length_t reg_data, out lru_fp_length_t out_data) {
			out_data = reg_data;
			reg_data = meta.key_evt_1;
		}
	};

	action insert_fp_action_2() {
		meta.key_evt_2 = insert_fp_salu_2.execute((bit<32>)hdr.lru_record.index[10:0]);
	}

	@stage(3)
	table insert_fp_table_2 {
		actions = {
			insert_fp_action_2;
		}
		size = 1;
		const default_action = insert_fp_action_2;
	}

/*********************** FP 3 **************************/
	Register<lru_fp_length_t, bucket_size_t>(BUCKET_SIZE) fp_3;
	RegisterAction<lru_fp_length_t, bucket_size_t, lru_fp_length_t>(fp_3) insert_fp_salu_3 = {
		void apply(inout lru_fp_length_t reg_data, out lru_fp_length_t out_data) {
			out_data = reg_data;
			reg_data = meta.key_evt_2;
		}
	};

	action insert_fp_action_3() {
		meta.key_evt_3 = insert_fp_salu_3.execute((bit<32>)hdr.lru_record.index[10:0]);
	}

	@stage(5)
	table insert_fp_table_3 {
		actions = {
			insert_fp_action_3;
		}
		size = 1;
		const default_action = insert_fp_action_3;
	}

/**************************** LRU DFA ********************************/
    Register<bit<8>, bit<32>>(BUCKET_SIZE) reg_dfa;
    RegisterAction<bit<8>, bit<32>, bit<8>>(reg_dfa) action_dfa_1 = {
        void apply (inout bit<8> reg_data, out bit<8> result) {
            result = reg_data;
        }
    };

    action table_action_dfa_1() {
        meta.dfa_sta = action_dfa_1.execute((bit<32>)hdr.lru_record.index[10:0]);
    }

    RegisterAction<bit<8>, bit<32>, bit<8>>(reg_dfa) action_dfa_2 = {
        void apply (inout bit<8> reg_data, out bit<8> result) {
            if (reg_data >= 4) {
                reg_data = reg_data ^ 1;
            }
            else {
                reg_data = reg_data ^ 3;
            }
            result = reg_data;
        }
    };

    action table_action_dfa_2() {
        meta.dfa_sta = action_dfa_2.execute((bit<32>)hdr.lru_record.index[10:0]);
    }

    RegisterAction<bit<8>, bit<32>, bit<8>>(reg_dfa) action_dfa_3 = {
        void apply (inout bit<8> reg_data, out bit<8> result) {
            if (reg_data <= 1) {
                reg_data = reg_data + 4;
            }
            else {
                reg_data = reg_data - 2;
            }
            result = reg_data;
        }
    };

    action table_action_dfa_3() {
        meta.dfa_sta = action_dfa_3.execute((bit<32>)hdr.lru_record.index[10:0]);
    }

    @stage(7) 
	table table_dfa_1 {
        actions = {
            table_action_dfa_1;
        }
        default_action = table_action_dfa_1;
    }


    @stage(7) 
	table table_dfa_2 {
        actions = {
            table_action_dfa_2;
        }
        default_action = table_action_dfa_2;
    }


    @stage(7) 
	table table_dfa_3 {
        actions = {
            table_action_dfa_3;
        }
        default_action = table_action_dfa_3;
    }

/**************************** LRU COUNTER ********************************/
/*********************** COUNTER 1 **************************/
	Register<lru_counter_length_t, bucket_size_t>(BUCKET_SIZE) counter_1;
	RegisterAction<lru_counter_length_t, bucket_size_t, lru_counter_length_t>(counter_1) insert_counter_salu_1 = {
		void apply(inout lru_fp_length_t reg_data, out lru_fp_length_t out_data) {
			if(meta.dfa_tsm == 4) {
				out_data = reg_data;
				reg_data = hdr.lru_record.counter;
			}
			else {
				out_data = 0;
				reg_data = reg_data + hdr.lru_record.counter;
			}
		}
	};

	action insert_counter_action_1() {
		hdr.lru_record.old_counter = insert_counter_salu_1.execute((bit<32>)hdr.lru_record.index[10:0]);
	}

	@stage(8)
	table insert_counter_table_1 {
		actions = {
			insert_counter_action_1;
		}
		size = 1;
		const default_action = insert_counter_action_1;
	}

/*********************** COUNTER 2 **************************/
	Register<lru_counter_length_t, bucket_size_t>(BUCKET_SIZE) counter_2;
	RegisterAction<lru_counter_length_t, bucket_size_t, lru_counter_length_t>(counter_2) insert_counter_salu_2 = {
		void apply(inout lru_fp_length_t reg_data, out lru_fp_length_t out_data) {
			if(meta.dfa_tsm == 4) {
				out_data = reg_data;
				reg_data = hdr.lru_record.counter;
			}
			else {
				out_data = 0;
				reg_data = reg_data + hdr.lru_record.counter;
			}
		}
	};

	action insert_counter_action_2() {
		hdr.lru_record.old_counter = insert_counter_salu_2.execute((bit<32>)hdr.lru_record.index[10:0]);
	}

	@stage(9)
	table insert_counter_table_2 {
		actions = {
			insert_counter_action_2;
		}
		size = 1;
		const default_action = insert_counter_action_2;
	}

/*********************** COUNTER 3 **************************/
	Register<lru_counter_length_t, bucket_size_t>(BUCKET_SIZE) counter_3;
	RegisterAction<lru_counter_length_t, bucket_size_t, lru_counter_length_t>(counter_3) insert_counter_salu_3 = {
		void apply(inout lru_fp_length_t reg_data, out lru_fp_length_t out_data) {
			if(meta.dfa_tsm == 4) {
				out_data = reg_data;
				reg_data = hdr.lru_record.counter;
			}
			else {
				out_data = 0;
				reg_data = reg_data + hdr.lru_record.counter;
			}
		}
	};

	action insert_counter_action_3() {
		hdr.lru_record.old_counter = insert_counter_salu_3.execute((bit<32>)hdr.lru_record.index[10:0]);
	}

	@stage(10)
	table insert_counter_table_3 {
		actions = {
			insert_counter_action_3;
		}
		size = 1;
		const default_action = insert_counter_action_3;
	}

// /*********************** POSTPROCESSING **************************/
// 	action set_is_replace_action() {
// 		hdr.
// 	}

// 	@stage(11)
// 	table set_is_replace_action {
// 		actions = {
// 			set_is_replace_action;
// 		}
// 		size = 1;
// 		const default_action = set_is_replace_action;
// 	}

// 	action set_no_replace_action() {
		
// 	}

// 	@stage(11)
// 	table set_is_replace_action {
// 		actions = {
// 			set_is_replace_action;
// 		}
// 		size = 1;
// 		const default_action = set_no_replace_action;
// 	}

/********************** EGRESS LOGIC *****************************/	
	apply {
        // if(hdr.lru_record.is_submit == 0) {        //if the packet is a submit packet, passby the hash processing, only recirculate
			lru_preprocessing_table.apply();
			lru_preprocessing_table_1.apply();

            if (meta.dfa_tsm == 0) {
                insert_fp_table_1.apply();
                if (meta.key_evt_1 == hdr.lru_record.fp) {
                    meta.dfa_tsm = 1;
                }
            }
            if (meta.dfa_tsm == 0) {
                insert_fp_table_2.apply();
                if (meta.key_evt_2 == hdr.lru_record.fp) {
                    meta.dfa_tsm = 2;
                }
            }
            if (meta.dfa_tsm == 0) {
                insert_fp_table_3.apply();
                if (meta.key_evt_3 == hdr.lru_record.fp) {
                    meta.dfa_tsm = 3;
                }
				else {
					meta.dfa_tsm = 4;
					hdr.lru_record.old_fp = meta.key_evt_3;
				}
            }

            if (meta.dfa_tsm == 1) {
                table_dfa_1.apply();
            } else if (meta.dfa_tsm == 2) {
                table_dfa_2.apply();
            } else {
                table_dfa_3.apply();
            }

            if (meta.dfa_sta == 4 || meta.dfa_sta == 1) {
                insert_counter_table_1.apply();
            } else if (meta.dfa_sta == 5 || meta.dfa_sta == 0) {
                insert_counter_table_2.apply();
            } else {
                insert_counter_table_3.apply();
            }

			//post processing
			if(hdr.lru_record.old_fp != 0&&hdr.lru_record.old_counter != 0) {
				hdr.lru_record.lru_is_replace = 1;
			}
        // }
    }


}

/*************************************************************************
 *********************  E G R E S S  D E P A R S E R *********************
*************************************************************************/
control EgressDeparser1(packet_out pkt,
	inout egress_header_t hdr,
	in egress_metadata_t meta,
	in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md)
{
	apply{
       pkt.emit(hdr); 
	}
}

control EgressDeparser2(packet_out pkt,
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
/* first ingress process used to filter, then recirculate*/
Pipeline(IngressParser1(),Ingress1(),IngressDeparser(),
EgressParser1(),Egress1(),EgressDeparser1()) pipe1;

/* recirculate ingress process used to hash, then send to the origin port and out*/
Pipeline(IngressParser2(),Ingress2(),IngressDeparser(),
EgressParser2(),Egress2(),EgressDeparser2()) pipe2;

Switch(pipe1,pipe2) main;

