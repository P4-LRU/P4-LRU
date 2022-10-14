#include "../driver/driver.h"
#include "../lib/b_plus_tree.h"
#include <bits/stdc++.h>

// #define CORRECTNESS
#define STORE_INSERT_MSG

using namespace std;

BPlusTree<16> b_plus_tree;
dpdk_driver driver;
string node_info_txt = "node_info.txt";
const int max_load_num = 4096;
const int compress_depth = 3;

#ifdef STORE_INSERT_MSG
ofstream msg_out("insert_msg.txt");
#endif

int nb_cache_hit = 0;
int nb_cache_miss = 0;

void exit_handler(int signo) {
  cout << endl << "cache_hit: " << nb_cache_hit << endl;
  cout << endl << "cache_miss: " << nb_cache_miss << endl;
  exit(0);
}

uint64_t remove_key_index_prefix(
    string key) // key_index : usertable + user + uint64 -> uint64
{
  key = key.substr(13);
  stringstream ss(key);
  uint64_t ret;
  ss >> ret;
  return ret;
}

void handle_insert(char *msg, uint16_t dst_qid) {
  std::string key_index;
  std::vector<std::pair<std::string, std::string>> values;

  msg += extract_key_index(msg, key_index);
  msg += extract_values(msg, values);

#ifdef STORE_INSERT_MSG
  msg_out << key_index << endl;
  for (auto kv : values)
    msg_out << kv.first << " " << kv.second << endl;
#endif

  b_plus_tree.insert(key_index, values);

  char *ret = new char[1472];
  unsigned msg_len = 0;

  msg_len += inject_msg_type(ret + msg_len, 0x0528);

  driver.send_pkt(ret, msg_len, dst_qid, 0);

  delete[] ret;
}

void handle_query(char *msg, uint16_t dst_qid) {
  Node *fbt_idx;
  int64_t tmp_idx;
  int8_t fbt_flag;

  std::string key_index;
  msg += extract_fbt_hdr(msg, tmp_idx, fbt_flag);
  msg += extract_key_index(msg, key_index);

  if (tmp_idx != 0) {
    nb_cache_hit += 1;
  } else {
    nb_cache_miss += 1;
  }
  // if ((nb_cache_hit + nb_cache_miss) % 100000 == 0) {
  //   cout << "cache hit : " << nb_cache_hit << endl;
  //   cout << "cache miss : " << nb_cache_miss << endl << endl;
  // }

  /***************************************************************************/
  fbt_idx = (Node *)tmp_idx;
  // fbt_idx = (Node *)0x0;
  /***************************************************************************/

  // cout << "Prev-search : " << remove_key_index_prefix(key_index) << " "
  //      << fbt_idx << " " << (int)fbt_flag << endl;
  std::vector<std::pair<std::string, std::string>> values =
      b_plus_tree.lookup(key_index, fbt_idx);
  // cout << "Prev-search : " << remove_key_index_prefix(key_index) << " "
  //      << fbt_idx << " " << (int)fbt_flag << endl
  //      << endl;

#ifdef CORRECTNESS
  std::vector<std::pair<std::string, std::string>> basic_values =
      b_plus_tree.lookup(key_index);

  if (values != basic_values) {
    cout << key_index << endl;
    cout << "fbt_idx" << hex << fbt_idx << endl;
    cout << "ours results:" << endl;
    for (auto kv : values)
      cout << "<" << kv.first << "," << kv.second << ">" << endl;
    cout << "basic results:" << endl;
    for (auto kv : basic_values)
      cout << "<" << kv.first << "," << kv.second << ">" << endl;
    cout << endl;
  }
#endif

  char *ret = new char[1472];
  unsigned msg_len = 0;

  tmp_idx = (int64_t)fbt_idx;

  msg_len += inject_msg_type(ret + msg_len, 0x0529);
  msg_len += inject_fbt_hdr(ret + msg_len, remove_key_index_prefix(key_index),
                            tmp_idx, fbt_flag);
  msg_len += inject_values(ret + msg_len, values);

  driver.send_pkt(ret, msg_len, dst_qid, 0);

  delete[] ret;
}

void handle_load2switch() {
  // b_plus_tree.load2switch(node_info_txt, max_load_num, compress_depth);
  // cout << "load2switch" << endl;

  // string str;
  // while (true)
  // {
  //   cin >> str;
  //   if (str == "done")
  //     break;
  // }

  char *ret = new char[1472];
  unsigned msg_len = 0;

  msg_len += inject_msg_type(ret + msg_len, 0x601);
  driver.send_pkt(ret, msg_len, 0, 0);

  delete[] ret;
}

int main(int argc, char *argv[]) {
  driver.init(argc, argv, 1);

  struct sigaction new_sa, old_sa;

  sigemptyset(&new_sa.sa_mask);
  new_sa.sa_flags = 0;
  new_sa.sa_handler = exit_handler;
  sigaction(SIGINT, &new_sa, &old_sa);

  char *msg = new char[1472];
  unsigned msg_type;
  unsigned msg_len;
  uint16_t recv_no;

  while (true) {
    msg_len = 0;

    driver.recv_pkt(msg, 1472, recv_no, 0);

    // for (int i = 0; i < 100; i++) {
    //   printf("%02x ", (unsigned)msg[i] & 0xff);
    //   if (i % 4 == 3)
    //     cout << endl;
    // }
    // cout << endl;

    msg_len += extract_msg_type(msg, msg_type);

    switch (msg_type) {
    case 0x0526:
      handle_insert(msg + msg_len, recv_no);
      break;
    case 0x0527:
      handle_query(msg + msg_len, recv_no);
      break;
    case 0x0600:
      handle_load2switch();
      break;
    default:
      printf("Warning: Undefined msg type %x\n", msg_type);
      break;
    }
  }
  delete[] msg;
}
