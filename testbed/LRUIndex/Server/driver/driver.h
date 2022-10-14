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

#include "flow_blocks.h"

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define MAX_QUEUE_NUM 16

static const struct rte_eth_conf port_conf_default = {
    .rxmode =
        {
            .max_rx_pkt_len = ETHER_MAX_LEN,
        },
};

class dpdk_driver {
public:
  uint16_t nb_queues;

  int init(int argc, char *argv[], int nb_threads = 1);

  void register_queue_id();
  void register_dest_port();
  uint16_t get_queue_id();
  uint16_t get_dest_port();

  void send_pkt(char *context, unsigned nb_bytes, uint16_t send_no = 0,
                uint16_t queue_id = 0);

  void recv_pkt(char *context, unsigned nb_bytes, uint16_t &recv_no,
                uint16_t queue_id = 0);

  void flow_init(uint16_t queue_id, uint16_t dest_port);

private:
  int port_init(uint16_t port, uint16_t rx_rings = 1, uint16_t tx_rings = 1);

  struct rte_mempool *mbuf_pool[MAX_QUEUE_NUM];
  std::map<std::thread::id, uint16_t> map_queue_id;
  std::map<std::thread::id, uint16_t> map_dest_port;
  std::mutex mutex_;
};

unsigned inject_msg_type(char *msg, unsigned msg_type);
unsigned inject_fbt_hdr(char *msg, uint64_t fbt_key, int64_t fbt_idx,
                        int8_t fbt_flag);
unsigned inject_key_index(char *msg, std::string &key_index);
unsigned
inject_values(char *msg,
              std::vector<std::pair<std::string, std::string>> &values);

unsigned extract_msg_type(char *msg, unsigned &msg_type);
unsigned extract_fbt_hdr(char *msg, int64_t &fbt_idx, int8_t &fbt_flag);
unsigned extract_key_index(char *msg, std::string &key_index);
unsigned
extract_values(char *msg,
               std::vector<std::pair<std::string, std::string>> &values);
