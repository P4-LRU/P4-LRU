/*
* lru for database cache
* do something for the query and query response packets
*/

/*
* Ingress: send the packets back
* Egress: do nothing
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
typedef bit<12> rand_len_t;
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
    bit<32> key;

    bit<32> value;
    bit<8> flag;
    rand_len_t rand;
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
    action send_back() {
        ig_tm_md.ucast_egress_port = ig_intr_md.ingress_port;
    }
    
    @stage(0)
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

/********************** INGRESS LOGIC *****************************/	
	apply {
        if(ig_intr_md.ingress_port == 128) {
            ig_tm_md.ucast_egress_port = 144;
        }
        else if(ig_intr_md.ingress_port == 144) {
            ig_tm_md.ucast_egress_port = 128;
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

