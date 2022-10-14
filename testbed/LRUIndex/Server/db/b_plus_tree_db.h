#ifndef YCSB_C_B_PLUS_TREE_DB_H_
#define YCSB_C_B_PLUS_TREE_DB_H_

#include "core/db.h"
#include "driver/driver.h"

#include <cassert>
#include <iostream>
#include <sched.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// #define CORRECTNESS

using namespace std;

namespace ycsbc {
class BPlusTreeDB : public DB {
public:
  BPlusTreeDB(int nb_threads = 1) : DB() {
    int argc = 5;
    char **argv;
    string *args;

    argv = new char *[argc];
    args = new string[argc];

    args[0] = "frontend";
    args[1] = "--file-prefix";
    args[2] = "frontend";
    args[3] = "-l";
    args[4] = "0-16";

    for (int i = 0; i < argc; i++) {
      argv[i] = (char *)args[i].c_str();
    }

    driver.init(argc, argv, nb_threads);
  }

  void Init();
  int Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields, std::vector<KVPair> &result);
  int Scan(const std::string &table, const std::string &key, int len,
           const std::vector<std::string> *fields,
           std::vector<std::vector<KVPair>> &result);
  int Update(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);
  int Insert(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);
  int Delete(const std::string &table, const std::string &key);
  int Load2Switch();

protected:
  int64_t root_idx;
  dpdk_driver driver;
  mutex mutex_;
  int init_count = 0;

#ifdef CORRECTNESS
  unordered_map<string, vector<KVPair>> ground_truth;
#endif
};

} // namespace ycsbc

#endif // YCSB_C_B_PLUS_TREE_DB_H_
