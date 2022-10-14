#include "driver.h"
#include <iostream>

using namespace std;

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

  retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);

  if (retval != 0)
    return retval;

  retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);

  if (retval != 0)
    return retval;

  for (q = 0; q < rx_rings; q++) {
    retval = rte_eth_rx_queue_setup(
        port, q, nb_rxd, rte_eth_dev_socket_id(port), NULL, mbuf_pool);
    if (retval < 0)
      return retval;
  }

  txconf = dev_info.default_txconf;
  txconf.offloads = port_conf.txmode.offloads;
  for (q = 0; q < tx_rings; q++) {
    retval = rte_eth_tx_queue_setup(port, q, nb_txd,
                                    rte_eth_dev_socket_id(port), &txconf);
    if (retval < 0)
      return retval;
  }

  retval = rte_eth_dev_start(port);
  if (retval < 0)
    return retval;

  struct ether_addr addr;
  rte_eth_macaddr_get(port, &addr);
  printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8
         " %02" PRIx8 " %02" PRIx8 "\n",
         port, addr.addr_bytes[0], addr.addr_bytes[1], addr.addr_bytes[2],
         addr.addr_bytes[3], addr.addr_bytes[4], addr.addr_bytes[5]);

  rte_eth_promiscuous_enable(port);

  return 0;
}

void dpdk_driver::send_pkt(char *context, unsigned nb_bytes) {
  uint16_t port = 0;

  struct rte_mbuf *mbuf;

  struct ether_hdr *hdr_eth;
  struct ipv4_hdr *hdr_ip;
  struct udp_hdr *hdr_udp;

  mbuf = rte_pktmbuf_alloc(mbuf_pool);

  char *payload = rte_pktmbuf_append(mbuf, nb_bytes);
  if (payload == NULL) {
    rte_exit(EXIT_FAILURE, "Error with rte_pktmbuf_append\n");
  }

  rte_memcpy(payload, context, nb_bytes);

  hdr_udp = (struct udp_hdr *)rte_pktmbuf_prepend(mbuf, sizeof(struct udp_hdr));
  hdr_udp->src_port = htons(0);
  hdr_udp->dst_port = htons(0);
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

  uint16_t nb_tx = rte_eth_tx_burst(port, 0, &mbuf, 1);

  if (unlikely(nb_tx < 1)) {
    rte_pktmbuf_free(mbuf);
  }
}

void dpdk_driver::recv_pkt(char *context, unsigned nb_bytes) {
  uint16_t port = 0;
  unsigned nb_rx_pkt;

  struct rte_mbuf *mbuf;

  struct ether_hdr *hdr_eth;
  struct ipv4_hdr *hdr_ip;
  struct udp_hdr *hdr_udp;

  uint16_t nb_rx;

  do {
    nb_rx = rte_eth_rx_burst(port, 0, &mbuf, 1);
  } while (unlikely(nb_rx < 1));

  hdr_eth = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);

  hdr_ip = (struct ipv4_hdr *)(hdr_eth + 1);

  hdr_udp = (struct udp_hdr *)(hdr_ip + 1);

  char *payload = (char *)(hdr_udp + 1);

  rte_memcpy(context, payload, nb_bytes);

  rte_pktmbuf_free(mbuf);
}

int dpdk_driver::init(int argc, char *argv[]) {
  unsigned nb_ports;

  int ret = rte_eal_init(argc, argv);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

  argc -= ret;
  argv += ret;

  nb_ports = rte_eth_dev_count_avail();

  if (nb_ports < 1)
    rte_exit(EXIT_FAILURE, "Error: number of ports must be greater than one\n");

  mbuf_pool =
      rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0,
                              RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

  if (mbuf_pool == NULL)
    rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

  if (port_init(0, 1, 1) != 0)
    rte_exit(EXIT_FAILURE, "Cannot init port\n");

  return ret;
}

unsigned inject_msg_type(char *msg, unsigned msg_type) {
  unsigned msg_len = 0;
  unsigned ubuf;

  ubuf = htonl(msg_type);
  memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
  msg_len += sizeof(unsigned);

  return msg_len;
}

unsigned inject_tab_hdr(char *msg, unsigned tab_key, unsigned tab_val) {
  unsigned msg_len = 0;
  unsigned ubuf;

  ubuf = htonl(tab_key);
  memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
  msg_len += sizeof(unsigned);

  ubuf = htonl(tab_val);
  memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
  msg_len += sizeof(unsigned);

  return msg_len;
}

unsigned inject_flow_hdr(char *msg, char *flow_key, unsigned flow_len) {
  unsigned msg_len = 0;
  unsigned ubuf;

  memcpy(msg + msg_len, flow_key, 13 * sizeof(char));
  msg_len += 13 * sizeof(char);

  ubuf = htonl(flow_len);
  memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
  msg_len += sizeof(unsigned);

  ubuf = 0;
  memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
  msg_len += sizeof(unsigned);

  ubuf = 0;
  memcpy(msg + msg_len, (char *)&ubuf, sizeof(unsigned));
  msg_len += sizeof(unsigned);

  return msg_len;
}

unsigned extract_msg_type(char *msg, unsigned &msg_type) {
  unsigned msg_len = 0;
  unsigned ubuf;

  ubuf = *(unsigned *)(msg + msg_len);
  msg_type = htonl(ubuf);
  msg_len += sizeof(unsigned);

  return msg_len;
}

unsigned extract_tab_hdr(char *msg, unsigned &tab_key, unsigned &tab_val) {
  unsigned msg_len = 0;
  unsigned ubuf;

  ubuf = *(unsigned *)(msg + msg_len);
  tab_key = htonl(ubuf);
  msg_len += sizeof(unsigned);

  ubuf = *(unsigned *)(msg + msg_len);
  tab_val = htonl(ubuf);
  msg_len += sizeof(unsigned);

  return msg_len;
}

unsigned extract_flow_hdr(char *msg, char *flow_key, unsigned &flow_len) {
  unsigned msg_len = 0;
  unsigned ubuf;

  memcpy(flow_key, msg + msg_len, 13 * sizeof(char));
  msg_len += 13 * sizeof(char);

  ubuf = *(unsigned *)(msg + msg_len);
  flow_len = htonl(ubuf);
  msg_len += sizeof(unsigned);

  ubuf = *(unsigned *)(msg + msg_len);
  msg_len += sizeof(unsigned);

  ubuf = *(unsigned *)(msg + msg_len);
  msg_len += sizeof(unsigned);

  return msg_len;
}

unsigned extract_load_hdr(char *msg, char *flow_key, unsigned &old_len,
                          unsigned &old_fp, unsigned &flow_fp) {
  unsigned msg_len = 0;
  unsigned ubuf;

  memcpy(flow_key, msg + msg_len, 13 * sizeof(char));
  msg_len += 13 * sizeof(char);

  ubuf = *(unsigned *)(msg + msg_len);
  old_len = htonl(ubuf);
  msg_len += sizeof(unsigned);

  ubuf = *(unsigned *)(msg + msg_len);
  old_fp = htonl(ubuf);
  msg_len += sizeof(unsigned);

  ubuf = *(unsigned *)(msg + msg_len);
  flow_fp = htonl(ubuf);
  msg_len += sizeof(unsigned);

  return msg_len;
}
