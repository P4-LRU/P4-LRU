/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include "driver.h"
#include <iostream>

using namespace std;
// #define DEBUG

/* basicfwd.c: Basic DPDK skeleton forwarding example. */

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
int dpdk_driver::port_init(uint16_t port, uint16_t rx_rings,
                           uint16_t tx_rings) {
  struct rte_eth_conf port_conf = port_conf_default;
  uint16_t nb_rxd = RX_RING_SIZE;
  uint16_t nb_txd = TX_RING_SIZE;
  int retval;
  uint16_t q;
  struct rte_eth_dev_info dev_info;
  struct rte_eth_txconf txconf;

  if (!rte_eth_dev_is_valid_port(port))
    return -1;

  rte_eth_dev_info_get(port, &dev_info);

  if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
    port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;

  /* Configure the Ethernet device. */
  retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);

  if (retval != 0)
    return retval;

  retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);

  if (retval != 0)
    return retval;

  /* Allocate and set up 1 RX queue per Ethernet port. */
  for (q = 0; q < rx_rings; q++) {
    retval = rte_eth_rx_queue_setup(
        port, q, nb_rxd, rte_eth_dev_socket_id(port), NULL, mbuf_pool[q]);
    if (retval < 0)
      return retval;
  }

  txconf = dev_info.default_txconf;
  txconf.offloads = port_conf.txmode.offloads;
  /* Allocate and set up 1 TX queue per Ethernet port. */
  for (q = 0; q < tx_rings; q++) {
    retval = rte_eth_tx_queue_setup(port, q, nb_txd,
                                    rte_eth_dev_socket_id(port), &txconf);
    if (retval < 0)
      return retval;
  }

  /* Start the Ethernet port. */
  retval = rte_eth_dev_start(port);
  if (retval < 0)
    return retval;

  /* Display the port MAC address. */
  struct ether_addr addr;
  rte_eth_macaddr_get(port, &addr);
  printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8
         " %02" PRIx8 " %02" PRIx8 "\n",
         port, addr.addr_bytes[0], addr.addr_bytes[1], addr.addr_bytes[2],
         addr.addr_bytes[3], addr.addr_bytes[4], addr.addr_bytes[5]);

  /* Enable RX in promiscuous mode for the Ethernet device. */
  rte_eth_promiscuous_enable(port);

  return 0;
}

void dpdk_driver::register_queue_id() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (map_queue_id.count(std::this_thread::get_id()) == 0) {
    map_queue_id[std::this_thread::get_id()] = map_queue_id.size();
  }
}

uint16_t dpdk_driver::get_queue_id() {
  return map_queue_id[std::this_thread::get_id()];
}

void dpdk_driver::register_dest_port() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (map_dest_port.count(std::this_thread::get_id()) == 0) {
    map_dest_port[std::this_thread::get_id()] = map_dest_port.size();
  }
}

uint16_t dpdk_driver::get_dest_port() {
  return map_dest_port[std::this_thread::get_id()];
}

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
void dpdk_driver::send_pkt(char *context, unsigned nb_bytes, uint16_t send_no,
                           uint16_t queue_id) {
  uint16_t port = 0;

  /* Get burst of RX packets, from first port of pair. */
  struct rte_mbuf *mbuf;

  struct ether_hdr *hdr_eth;
  struct ipv4_hdr *hdr_ip;
  struct udp_hdr *hdr_udp;

  mbuf = rte_pktmbuf_alloc(mbuf_pool[queue_id]);

  char *payload = rte_pktmbuf_append(mbuf, nb_bytes);
  if (payload == NULL) {
    rte_exit(EXIT_FAILURE, "Error with rte_pktmbuf_append\n");
  }

  rte_memcpy(payload, context, nb_bytes);

  hdr_udp = (struct udp_hdr *)rte_pktmbuf_prepend(mbuf, sizeof(struct udp_hdr));
  hdr_udp->src_port = htons(0);
  hdr_udp->dst_port = htons(send_no); // we use udp's src_port to carry no.
  hdr_udp->dgram_len = htons(nb_bytes + sizeof(struct udp_hdr));
  hdr_udp->dgram_cksum = 0;

  hdr_ip =
      (struct ipv4_hdr *)rte_pktmbuf_prepend(mbuf, sizeof(struct ipv4_hdr));
  hdr_ip->version_ihl = 0x45;
  hdr_ip->type_of_service = 0;
  hdr_ip->total_length =
      htons(nb_bytes + sizeof(struct udp_hdr) + sizeof(struct ipv4_hdr));
  hdr_ip->packet_id = 0;
  hdr_ip->fragment_offset = 0;
  hdr_ip->time_to_live = 64;
  hdr_ip->next_proto_id = IPPROTO_UDP;
  hdr_ip->hdr_checksum = 0;
  hdr_ip->src_addr = htonl(0);
  hdr_ip->dst_addr = htonl(0);

  hdr_eth =
      (struct ether_hdr *)rte_pktmbuf_prepend(mbuf, sizeof(struct ether_hdr));
  memset(&hdr_eth->s_addr, 0x0, ETHER_ADDR_LEN);
  memset(&hdr_eth->d_addr, 0x0, ETHER_ADDR_LEN);
  hdr_eth->ether_type = htons(ETHER_TYPE_IPv4);

  hdr_udp->dgram_cksum = rte_ipv4_udptcp_cksum(hdr_ip, hdr_udp);
  hdr_ip->hdr_checksum = rte_ipv4_cksum(hdr_ip);

  /* Send burst of TX packets, to second port of pair. */
  uint16_t nb_tx = rte_eth_tx_burst(port, queue_id, &mbuf, 1);

  /* Free any unsent packets. */
  if (unlikely(nb_tx < 1)) {
    rte_pktmbuf_free(mbuf);
  }
}

void dpdk_driver::recv_pkt(char *context, unsigned nb_bytes, uint16_t &recv_no,
                           uint16_t queue_id) {
  uint16_t port = 0;
  unsigned nb_rx_pkt;

  /* Get burst of RX packets, from first port of pair. */
  struct rte_mbuf *mbuf;

  struct ether_hdr *hdr_eth;
  struct ipv4_hdr *hdr_ip;
  struct udp_hdr *hdr_udp;

  /* Send burst of TX packets, to second port of pair. */
  uint16_t nb_rx;

  /* Free any unsent packets. */
  do {
    nb_rx = rte_eth_rx_burst(port, queue_id, &mbuf, 1);
  } while (unlikely(nb_rx < 1));

  hdr_eth = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);

  hdr_ip = (struct ipv4_hdr *)(hdr_eth + 1);

  hdr_udp = (struct udp_hdr *)(hdr_ip + 1);
  recv_no = htons(hdr_udp->dst_port);

  char *payload = (char *)(hdr_udp + 1);

  rte_memcpy(context, payload, nb_bytes);

  rte_pktmbuf_free(mbuf);
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int dpdk_driver::init(int argc, char *argv[], int nb_threads) {
  unsigned nb_ports;
  uint16_t portid = 0;
  uint16_t q;

  /* Initialize the Environment Abstraction Layer (EAL). */
  int ret = rte_eal_init(argc, argv);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

  argc -= ret;
  argv += ret;

  nb_ports = rte_eth_dev_count_avail();

  if (nb_ports < 1)
    rte_exit(EXIT_FAILURE, "Error: number of ports must be greater than one\n");

  /* Creates a new mempool in memory to hold the mbufs. */
  nb_queues = nb_threads;
  for (q = 0; q < nb_queues; ++q) {
    mbuf_pool[q] = rte_pktmbuf_pool_create(
        (std::string("MBUF_POOL") + std::to_string(q)).c_str(), NUM_MBUFS,
        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pool[q] == NULL)
      rte_exit(EXIT_FAILURE, "Cannot create mbuf pool %" PRIu16 "\n", q);
  }

  /* Initialize all ports. */
  if (port_init(portid, nb_queues, nb_queues) != 0)
    rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n", portid);

  return ret;
}

void dpdk_driver::flow_init(uint16_t queue_id, uint16_t dest_port) {
  struct rte_flow *flow;
  struct rte_flow_error error;
  uint16_t port_id = 0;

  printf("dpdk::driver::flow_init(queue_id: %d, dest_port: %d)\n", queue_id,
         dest_port);

  /* create flow for send packet with */
  flow = generate_udp_flow(port_id, queue_id, 0x0, EMPTY_IP_MASK, 0x0,
                           EMPTY_IP_MASK, 0x0, EMPTY_PORT_MASK, dest_port,
                           FULL_PORT_MASK, &error);
  if (!flow) {
    printf("Flow can't be created %d message: %s\n", error.type,
           error.message ? error.message : "(no stated reason)");
    rte_exit(EXIT_FAILURE, "error in creating flow");
  }
}

unsigned inject_msg_type(char *msg, unsigned msg_type) {
  unsigned msg_len = 0;
  unsigned ubuf;

  ubuf = htonl(msg_type);
  memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
  msg_len += sizeof(unsigned);

#ifdef DEBUG
  printf("message type: [ %d ]\n", ubuf);
#endif

  return msg_len;
}

unsigned inject_fbt_hdr(char *msg, uint64_t fbt_key, int64_t fbt_idx,
                        int8_t fbt_flag) {
  unsigned msg_len = 0;
  unsigned ubuf;

  uint32_t key_hi = htonl(fbt_key >> 32);
  uint32_t key_lo = htonl(fbt_key & 0xffffffff);

  ubuf = key_hi;
  memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
  msg_len += sizeof(unsigned);

  ubuf = key_lo;
  memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
  msg_len += sizeof(unsigned);

  // printf("key_hi, key_lo = %u, %u\n", ntohl(key_hi), ntohl(key_lo));

  int8_t *in_idx = (int8_t *)&fbt_idx;
  memcpy(msg + msg_len + 0, in_idx + 5, sizeof(int8_t));
  memcpy(msg + msg_len + 1, in_idx + 4, sizeof(int8_t));
  memcpy(msg + msg_len + 2, in_idx + 3, sizeof(int8_t));
  memcpy(msg + msg_len + 3, in_idx + 2, sizeof(int8_t));
  memcpy(msg + msg_len + 4, in_idx + 1, sizeof(int8_t));
  memcpy(msg + msg_len + 5, in_idx + 0, sizeof(int8_t));
  msg_len += 6;

  // cout << "inject: ";
  // for (int i = 1; i <= 6; i++) {
  //   cout << (int)(uint8_t) * (msg + msg_len - i) << " ";
  // }
  // cout << endl;

  memcpy(msg + msg_len, (char *)&fbt_flag, sizeof(int8_t));
  msg_len += 1;

#ifdef DEBUG
  printf("fbt_key: [ %u, %u ]\n", key_hi, key_lo);
#endif

  // cout << "msg_len: " << msg_len << endl;
  return msg_len;
}

unsigned inject_key_index(char *msg, std::string &key_index) {
  unsigned msg_len = 0;
  unsigned ubuf;

  ubuf = key_index.length();
  memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
  msg_len += sizeof(unsigned); // key_index_len

  memcpy(msg + msg_len, key_index.c_str(), key_index.length());
  msg_len += key_index.length(); // key_index

#ifdef DEBUG
  fprintf(stderr, "key_index length: %d; key_index: %s\n", ubuf,
          key_index.c_str());
#endif

  return msg_len;
}

unsigned
inject_values(char *msg,
              std::vector<std::pair<std::string, std::string>> &values) {
  unsigned msg_len = 0;
  unsigned ubuf;

  ubuf = values.size();
  memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
  msg_len += sizeof(unsigned); // values_size

  for (size_t i = 0; i < values.size(); i++) {
    ubuf = values[i].first.length();
    memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
    msg_len += sizeof(unsigned); // values_k_len

    memcpy(msg + msg_len, values[i].first.c_str(), values[i].first.size());
    msg_len += values[i].first.size(); // values_k

#ifdef DEBUG
    fprintf(stderr, "K: (length: %d) %s\n", ubuf, values[i].first.c_str());
#endif

    ubuf = values[i].second.length();
    memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
    msg_len += sizeof(unsigned); // values_v_len

    memcpy(msg + msg_len, values[i].second.c_str(), values[i].second.size());
    msg_len += values[i].second.size(); // values_v

#ifdef DEBUG
    fprintf(stderr, "V: (length: %d) %s\n", ubuf, values[i].second.c_str());
#endif
  }

  return msg_len;
}

unsigned extract_msg_type(char *msg, unsigned &msg_type) {
  unsigned msg_len = 0;
  unsigned ubuf;

  ubuf = *(unsigned *)(msg + msg_len);
  msg_type = htonl(ubuf);
  msg_len += sizeof(unsigned); // Insert

#ifdef DEBUG
  printf("message type: [ %d ]\n", ubuf);
#endif

  return msg_len;
}
unsigned extract_fbt_hdr(char *msg, int64_t &fbt_idx, int8_t &fbt_flag) {
  unsigned msg_len = 0;
  unsigned ubuf;
  int8_t in_idx[8] = {0};

  ubuf = *(unsigned *)(msg + msg_len);
  unsigned key_hi = htonl(ubuf);
  msg_len += sizeof(unsigned);

  ubuf = *(unsigned *)(msg + msg_len);
  unsigned key_lo = htonl(ubuf);
  msg_len += sizeof(unsigned);

  // memcpy((char*)&fbt_idx, msg + msg_len, 6);

  in_idx[0] = *(msg + msg_len + 5);
  in_idx[1] = *(msg + msg_len + 4);
  in_idx[2] = *(msg + msg_len + 3);
  in_idx[3] = *(msg + msg_len + 2);
  in_idx[4] = *(msg + msg_len + 1);
  in_idx[5] = *(msg + msg_len + 0);
  fbt_idx = *(int64_t *)in_idx;
  msg_len += 6;

  // cout << "extract: ";
  // for (int i = 0; i < 8; i++) {
  //   cout << (int)(uint8_t)in_idx[i] << " ";
  // }
  // cout << endl;

  fbt_flag = *(int8_t *)(msg + msg_len);
  msg_len += 1;

#ifdef DEBUG
  printf("key_hi: [ %u ]\n", key_hi);
  printf("key_lo: [ %u ]\n", key_lo);
  printf("fbt_idx: [ %u ]\n", fbt_idx);
  printf("fbt_idx: [ %u ]\n", fbt_flag);
#endif

  // cout << "msg_len: " << msg_len << endl;
  return msg_len;
}

unsigned extract_key_index(char *msg, std::string &key_index) {
  unsigned msg_len = 0;
  unsigned ubuf;

  // for (int i = 0; i < 100; i++)
  // {
  //   printf("%02x ", (unsigned)msg[i] & 0xff);
  //   if (i % 4 == 3)
  //     cout << endl;
  // }
  // cout << endl;

  ubuf = *(unsigned *)(msg + msg_len);
  msg_len += sizeof(unsigned); // key_index_len

  key_index.assign(msg + msg_len, ubuf);
  msg_len += key_index.length(); // key_index

#ifdef DEBUG
  fprintf(stderr, "key_index length: %d; key_index: %s\n", ubuf,
          key_index.c_str());
#endif

  return msg_len;
}

unsigned
extract_values(char *msg,
               std::vector<std::pair<std::string, std::string>> &values) {
  unsigned msg_len = 0;
  unsigned ubuf;

  ubuf = *(unsigned *)(msg + msg_len);
  msg_len += sizeof(unsigned); // values_size

  values.resize(ubuf);

  for (size_t i = 0; i < values.size(); i++) {
    ubuf = *(unsigned *)(msg + msg_len);
    msg_len += sizeof(unsigned); // values_k_len

    values[i].first = std::string(msg + msg_len, ubuf);
    msg_len += values[i].first.size(); // values_k

#ifdef DEBUG
    fprintf(stderr, "K: (length: %d) %s\n", ubuf, values[i].first.c_str());
#endif

    ubuf = *(unsigned *)(msg + msg_len);
    msg_len += sizeof(unsigned); // values_v_len

    values[i].second = std::string(msg + msg_len, ubuf);
    msg_len += values[i].second.size(); // values_v

#ifdef DEBUG
    fprintf(stderr, "V: (length: %d) %s\n", ubuf, values[i].second.c_str());
#endif
  }

  return msg_len;
}
