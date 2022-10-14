#include "../driver/driver.h"
#include <arpa/inet.h>
#include <bits/stdc++.h>

using namespace std;

dpdk_driver driver;

int nb_recv = 0;
double nb_byte = 0;

void exit_handler(int signo) {
  cout << endl << "nb_recv: " << nb_recv << endl;
  cout << endl << "nb_byte: " << nb_byte << endl;
  exit(0);
}

void recv_caida_pkt() {
  char *msg = new char[1500];
  unsigned msg_len = 0;

  driver.recv_pkt(msg, 1500);

  char flow_key[13];
  unsigned flow_len;

  memset(flow_key, 0, sizeof(flow_key));
  // msg_len += extract_flow_hdr(msg + msg_len, flow_key, flow_len);
  msg_len += extract_flow_hdr(msg + msg_len, flow_key, flow_len);

  nb_recv += 1;
  nb_byte += flow_len;

  delete[] msg;
}

#pragma pack(push, 1)
struct packet {
  char key[13];
  uint16_t len;
  double ts;
};
#pragma pack(pop)

int main(int argc, char *argv[]) {
  driver.init(argc, argv);

  struct sigaction new_sa, old_sa;

  sigemptyset(&new_sa.sa_mask);
  new_sa.sa_flags = 0;
  new_sa.sa_handler = exit_handler;
  sigaction(SIGINT, &new_sa, &old_sa);

  while (true) {
    recv_caida_pkt();
  }
}
