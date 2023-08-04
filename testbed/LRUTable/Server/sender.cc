#include "../../driver/driver.h"
#include <arpa/inet.h>
#include <bits/stdc++.h>

using namespace std;

dpdk_driver driver;

int nb_send = 0;
auto t1 = chrono::high_resolution_clock::now();
auto t2 = chrono::high_resolution_clock::now();

void exit_handler(int signo) {
  t2 = chrono::high_resolution_clock::now();
  cout << endl << "nb_send: " << nb_send << endl;
  cout << endl
       << "average latency: "
       << 1.0 * chrono::duration_cast<chrono::microseconds>(t2 - t1).count() /
              nb_send
       << endl;
  exit(0);
}

string ip_2_address(unsigned ip) {
  string ret = "";
  ret += to_string((ip >> 24) & 0xFF);
  ret += ".";
  ret += to_string((ip >> 16) & 0xFF);
  ret += ".";
  ret += to_string((ip >> 8) & 0xFF);
  ret += ".";
  ret += to_string((ip >> 0) & 0xFF);
  return ret;
}

void send_caida_pkt(unsigned tab_key, unsigned tab_val) {
  char *msg = new char[1500];
  char *ret = new char[1500];
  unsigned msg_len = 0;
  unsigned ret_len = 0;

  memset(msg, 0, sizeof(msg));

  msg_len += inject_msg_type(msg + msg_len, 0x0527);
  msg_len += inject_tab_hdr(msg + msg_len, tab_key, tab_val);

  // cout << "send: " << ip_2_address(tab_key) << " " << ip_2_address(tab_val)
  //      << endl;

  driver.send_pkt(msg, 1472);
  driver.recv_pkt(ret, 1472);

  unsigned type;

  ret_len += extract_msg_type(ret + ret_len, type);
  assert(type == 0x0529 || type == 0x0527);
  ret_len += extract_tab_hdr(ret + ret_len, tab_key, tab_val);

  // cout << "recv: " << ip_2_address(tab_key) << " " << ip_2_address(tab_val)
  //      << endl;

  nb_send += 1;

  delete[] msg;
  delete[] ret;
}

#pragma pack(push, 1)
struct packet {
  unsigned src;
  unsigned dst;
  char key[5];
  uint16_t len;
  double ts;
};
#pragma pack(pop)

vector<packet> vp;

int main(int argc, char *argv[]) {
  driver.init(argc, argv);
  ifstream caida("/home/lab1806/CAIDA_BYTE_TSTAMP_2018/normalized_060.dat", ios::binary);

  struct sigaction new_sa, old_sa;

  sigemptyset(&new_sa.sa_mask);
  new_sa.sa_flags = 0;
  new_sa.sa_handler = exit_handler;
  sigaction(SIGINT, &new_sa, &old_sa);

  packet p;
  int total_num = 0;
  while (caida.read((char *)&p, sizeof(packet))) {
    vp.push_back(p);
    total_num += 1;
  }

  cout << "dataset read: "<<total_num << endl;

  // while (true) {
  for (int i = 0; i < vp.size(); i++) {
    send_caida_pkt(ntohl(vp[i].src), ntohl(vp[i].dst));
  }
  // }

  exit_handler(0);
}
