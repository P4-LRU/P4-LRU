#ifndef FLOW_BLOCKS_H_
#define FLOW_BLOCKS_H_

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/types.h>

#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_flow.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_net.h>

#define FULL_IP_MASK 0xffffffff /* full mask */
#define EMPTY_IP_MASK 0x0       /* empty mask */
#define FULL_PORT_MASK 0xffff
#define EMPTY_PORT_MASK 0x0

struct rte_flow *generate_udp_flow(uint16_t port_id, uint16_t rx_q,
                                   uint32_t src_ip, uint32_t src_ip_mask,
                                   uint32_t dest_ip, uint32_t dest_ip_mask,
                                   uint32_t src_port, uint32_t src_port_mask,
                                   uint32_t dest_port, uint32_t dest_port_mask,
                                   struct rte_flow_error *error);

#endif // FLOW_BLOCKS_H_
