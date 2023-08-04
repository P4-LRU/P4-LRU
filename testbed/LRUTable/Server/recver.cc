#include "../../driver/driver.h"
#include <arpa/inet.h>
#include <bits/stdc++.h>

using namespace std;

dpdk_driver driver;

unordered_map<unsigned, unsigned> table;

int nb_recv = 0;

void exit_handler(int signo) {
  cout << endl << "nb_recv: " << nb_recv << endl;
  cout << endl << "table_size: " << table.size() << endl;
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

void recv_caida_pkt() {
  char *msg = new char[1500];
  char *ret = new char[1500];
  memset(ret, 0, sizeof(ret));

  unsigned msg_len = 0;
  unsigned ret_len = 0;

  driver.recv_pkt(msg, 1472);

  unsigned type;
  unsigned tab_key;
  unsigned tab_val;

  msg_len += extract_msg_type(msg + msg_len, type);
  assert(type == 0x0527);
  msg_len += extract_tab_hdr(msg + msg_len, tab_key, tab_val);

  table[tab_key] = tab_key;

  // cout << "recv: " << ip_2_address(tab_key) << " " << ip_2_address(tab_val)
  //      << endl;

  ret_len += inject_msg_type(ret + ret_len, 0x0529);
  ret_len += inject_tab_hdr(ret + ret_len, tab_key, table[tab_key]);

  // cout << "send: " << ip_2_address(tab_key) << " " << ip_2_address(tab_key)
  //      << endl;

  driver.send_pkt(ret, 1472);

  nb_recv += 1;
  delete[] msg;
  delete[] ret;
}

const int T_entry = 10000000;

int main(int argc, char *argv[]) {
  driver.init(argc, argv);

  struct sigaction new_sa, old_sa;

  sigemptyset(&new_sa.sa_mask);
  new_sa.sa_flags = 0;
  new_sa.sa_handler = exit_handler;
  sigaction(SIGINT, &new_sa, &old_sa);

  for (int i = 0; i < T_entry; i++) {
    unsigned src = rand();
    unsigned dst = rand();
    table[src] = dst;
  }
  cout << endl << "table init." << endl;

  while (true) {
    recv_caida_pkt();
  }
}
