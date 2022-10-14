#include "db/b_plus_tree_db.h"
#include <fstream>

using namespace std;
using ycsbc::BPlusTreeDB;
using ycsbc::DB;

void BPlusTreeDB::Init() {
  lock_guard<mutex> lock(mutex_);
  ++init_count;

  driver.register_queue_id();
  driver.register_dest_port();

  if (init_count <= driver.nb_queues)
    driver.flow_init(driver.get_queue_id(), driver.get_dest_port());

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(driver.get_dest_port() + 1, &cpuset);
  if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == -1) {
    printf("Warning: failed to bind %d-th thread to %d-th CPU core\n",
           driver.get_dest_port(), driver.get_dest_port() + 1);
  }

  cout << "A new thread begins working." << endl;
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

int BPlusTreeDB::Read(const std::string &table, const std::string &key,
                      const std::vector<std::string> *fields,
                      std::vector<KVPair> &result) {
  string key_index(table + key);

  char *msg = new char[1472];
  unsigned msg_len = 0;
  unsigned msg_type;
  uint16_t queue_id = driver.get_queue_id();
  uint16_t dest_port = driver.get_dest_port();

  msg_len += inject_msg_type(msg + msg_len, 0x0527);
  msg_len += inject_fbt_hdr(msg + msg_len, remove_key_index_prefix(key_index),
                            0x0, 0x0);
  msg_len += inject_key_index(msg + msg_len, key_index);
  driver.send_pkt(msg, msg_len, dest_port, queue_id);

  driver.recv_pkt(msg, 1472, dest_port, queue_id);
  msg_len = 0;
  msg_len += extract_msg_type(msg + msg_len, msg_type);
  assert(msg_type == 0x0529);

  int64_t tmp_idx;
  int8_t tmp_flag;
  msg_len += extract_fbt_hdr(msg + msg_len, tmp_idx, tmp_flag);
  msg_len += extract_values(msg + msg_len, result);

  // cout << key_index << " " << (void *)tmp_idx << endl;

#ifdef CORRECTNESS
  auto gt_result = ground_truth[key_index];
  if (result != gt_result) {
    cout << key_index << endl;
    cout << "result:" << endl;
    for (auto kv : result)
      cout << "<" << kv.first << "," << kv.second << ">" << endl;
    cout << "ground truth:" << endl;
    for (auto kv : gt_result)
      cout << "<" << kv.first << "," << kv.second << ">" << endl;
    cout << endl;
  }
#endif

  delete[] msg;
  return DB::kOK;
}

int BPlusTreeDB::Scan(const std::string &table, const std::string &key, int len,
                      const std::vector<std::string> *fields,
                      std::vector<std::vector<KVPair>> &result) {
  throw "Scan: function not implemented!";
}

int BPlusTreeDB::Update(const std::string &table, const std::string &key,
                        std::vector<KVPair> &values) {
  throw "Update: function not implemented!";
}

int BPlusTreeDB::Insert(const std::string &table, const std::string &key,
                        std::vector<KVPair> &values) {
  string key_index(table + key);
  string recv_key_index;

  char *msg = new char[1472];
  unsigned msg_len = 0;
  unsigned msg_type;
  uint16_t queue_id = driver.get_queue_id();
  uint16_t dest_port = driver.get_dest_port();

  // cout << "send packet from queue " << queue_id << ", msg = " << key_index
  //      << endl;

  msg_len += inject_msg_type(msg + msg_len, 0x0526);
  msg_len += inject_key_index(msg + msg_len, key_index);
  msg_len += inject_values(msg + msg_len, values);
  driver.send_pkt(msg, msg_len, dest_port, queue_id);

  driver.recv_pkt(msg, 1472, dest_port, queue_id);
  msg_len = 0;
  msg_len += extract_msg_type(msg + msg_len, msg_type);
  assert(msg_type == 0x0528);
  // msg_len += extract_key_index(msg + msg_len, recv_key_index);

  // cout << "recv packet from queue " << queue_id << ", msg_type = " <<
  // msg_type << endl;

  // if (key_index != recv_key_index)
  //   cout << "Error: " << key_index << " " << recv_key_index << endl;
  // else
  //   cout << "Correct: " << key_index << " " << recv_key_index << endl;

#ifdef CORRECTNESS
  lock_guard<mutex> lock(mutex_);
  ground_truth[key_index] = values;
#endif

  delete[] msg;
  return DB::kOK;
}

int BPlusTreeDB::Delete(const std::string &table, const std::string &key) {
  throw "Delete: function not implemented!";
}

int BPlusTreeDB::Load2Switch() {
  char *msg = new char[1472];
  unsigned msg_len = 0;
  unsigned msg_type;
  uint16_t recv_no;

  msg_len += inject_msg_type(msg + msg_len, 0x0600);
  driver.send_pkt(msg, msg_len, 0, 0);

  driver.recv_pkt(msg, 1472, recv_no, 0);
  msg_len = 0;
  msg_len += extract_msg_type(msg + msg_len, msg_type);
  assert(msg_type == 0x0601);

  return DB::kOK;
}
