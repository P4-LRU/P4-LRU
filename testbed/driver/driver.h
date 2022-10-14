/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

extern "C" {
#include <inttypes.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_udp.h>
#include <stdint.h>
#include <unistd.h>
}
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define MAX_QUEUE_NUM 1

static const struct rte_eth_conf port_conf_default = {
    .rxmode =
        {
            .max_rx_pkt_len = ETHER_MAX_LEN,
        },
};

class dpdk_driver {
public:
  uint16_t nb_queues;

  int init(int argc, char *argv[]);

  void send_pkt(char *context, unsigned nb_bytes);

  void recv_pkt(char *context, unsigned nb_bytes);

  int port_init(uint16_t port, uint16_t rx_rings, uint16_t tx_rings);

private:
  struct rte_mempool *mbuf_pool;
};

unsigned inject_msg_type(char *msg, unsigned msg_type);
unsigned inject_flow_hdr(char *msg, char *flow_key, unsigned flow_len);
unsigned inject_tab_hdr(char *msg, unsigned tab_key, unsigned tab_val);

unsigned extract_msg_type(char *msg, unsigned &msg_type);
unsigned extract_flow_hdr(char *msg, char *flow_key, unsigned &flow_len);
unsigned extract_load_hdr(char *msg, char *flow_key, unsigned &old_len,
                          unsigned &old_fp, unsigned &flow_fp);
unsigned extract_tab_hdr(char *msg, unsigned &tab_key, unsigned &tab_val);
