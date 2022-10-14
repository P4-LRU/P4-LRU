#ifndef YCSB_C_B_PLUS_TREE_H_
#define YCSB_C_B_PLUS_TREE_H_

#include <algorithm>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>

using namespace std;

typedef string KEY_TYPE;
typedef vector<pair<string, string>> VAL_TYPE;

class Node {
public:
  bool isLeaf = true;
  Node *parent = NULL;
  vector<KEY_TYPE> key;
  vector<VAL_TYPE> val; // only leaf records val
  vector<Node *> child;
  uint32_t access_count = 0;
};

template <uint32_t M> class BPlusTree {
public:
  void insert(KEY_TYPE in_key, VAL_TYPE in_val) {
    lock_guard<mutex> lock(mutex_);
    Node *cur = root;

    while (!cur->isLeaf) {
      auto it = lower_bound(cur->key.begin(), cur->key.end(), in_key);
      cur = cur->child[it - cur->key.begin()];
    }

    auto p = lower_bound(cur->key.begin(), cur->key.end(), in_key) -
             cur->key.begin();
    cur->key.insert(cur->key.begin() + p, in_key);
    cur->val.insert(cur->val.begin() + p, in_val);

    overflow(cur);
  }

  VAL_TYPE lookup(KEY_TYPE in_key, Node *&cur) {
    if (cur == NULL)
      cur = root;
    ++cur->access_count;

    while (!cur->isLeaf) {
      auto it = lower_bound(cur->key.begin(), cur->key.end(), in_key);
      cur = cur->child[it - cur->key.begin()];
      ++cur->access_count;
    }

    auto it = lower_bound(cur->key.begin(), cur->key.end(), in_key);
    if (it == cur->key.end())
      return VAL_TYPE();
    return cur->val[it - cur->key.begin()];
  }

  void load2switch(string filename, uint32_t max_load_num,
                   uint32_t compress_depth) {
    ofstream fout(filename);
    vector<vector<Node *>> cached_nodes(depth - 1);
    vector<vector<KEY_TYPE>> cached_keys(compress_depth + 1);

    /* For layer < compress_depth, nodes are compressed into a "fat" node and
     * printed */
    cached_nodes[0].push_back(root);

    for (int i = 1; i <= compress_depth; ++i)
      for (int j = 0; j < cached_nodes[i - 1].size(); ++j) {
        cached_keys[i].insert(cached_keys[i].end(),
                              cached_nodes[i - 1][j]->key.begin(),
                              cached_nodes[i - 1][j]->key.end());
        if (j < cached_keys[i - 1].size())
          cached_keys[i].push_back(cached_keys[i - 1][j]);
        cached_nodes[i].insert(cached_nodes[i].end(),
                               cached_nodes[i - 1][j]->child.begin(),
                               cached_nodes[i - 1][j]->child.end());
      }

    print_info(root, 0, cached_keys[compress_depth],
               cached_nodes[compress_depth], fout);
    cout << "Layer 0-" << compress_depth - 1
         << ": access_count = " << root->access_count << endl;

    /* For layer > compress_depth, only frequent-accessed nodes are printed */
    sort(cached_nodes[compress_depth].begin(),
         cached_nodes[compress_depth].end(), [](Node *n1, Node *n2) {
           return n1->access_count > n2->access_count;
         });
    if (cached_nodes[compress_depth].size() > max_load_num)
      cached_nodes[compress_depth].resize(max_load_num);

    for (int i = compress_depth + 1; i < depth - 1; ++i) {
      for (auto p : cached_nodes[i - 1])
        for (auto ch : p->child)
          cached_nodes[i].push_back(ch);

      sort(cached_nodes[i].begin(), cached_nodes[i].end(),
           [](Node *n1, Node *n2) {
             return n1->access_count > n2->access_count;
           });
      if (cached_nodes[i].size() > max_load_num)
        cached_nodes[i].resize(max_load_num);
    }

    for (int i = compress_depth; i < depth - 1; ++i) {
      int access_count = 0;
      for (auto n : cached_nodes[i]) {
        print_info(n, i - compress_depth + 1, n->key, n->child, fout);
        access_count += n->access_count;
      }
      cout << "Layer " << i << ": access_count = " << access_count << endl;
    }

    fout.close();
  }

protected:
  Node *root = new Node;
  uint32_t depth = 1;
  mutex mutex_;

  void overflow(Node *cur) {
    if (cur->key.size() >= M) {
      if (cur == root) {
        root = new Node;
        root->isLeaf = false;
        root->child.push_back(cur);
        cur->parent = root;
        ++depth;
      }

      Node *new_node = new Node;
      new_node->parent = cur->parent;
      new_node->isLeaf = cur->isLeaf;

      if (cur->isLeaf) {
        new_node->key.insert(new_node->key.begin(), cur->key.begin() + M / 2,
                             cur->key.end());
        cur->key.erase(cur->key.begin() + M / 2, cur->key.end());
        new_node->val.insert(new_node->val.begin(), cur->val.begin() + M / 2,
                             cur->val.end());
        cur->val.erase(cur->val.begin() + M / 2, cur->val.end());

        auto p = lower_bound(cur->parent->key.begin(), cur->parent->key.end(),
                             new_node->key[0]) -
                 cur->parent->key.begin();
        cur->parent->key.insert(cur->parent->key.begin() + p, cur->key.back());
        cur->parent->child.insert(cur->parent->child.begin() + p + 1, new_node);
      } else {
        new_node->key.insert(new_node->key.begin(),
                             cur->key.begin() + M / 2 + 1, cur->key.end());
        auto new_key = cur->key[M / 2];
        cur->key.erase(cur->key.begin() + M / 2, cur->key.end());
        new_node->child.insert(new_node->child.begin(),
                               cur->child.begin() + M / 2 + 1,
                               cur->child.end());
        cur->child.erase(cur->child.begin() + M / 2 + 1, cur->child.end());
        for (auto ch : new_node->child)
          ch->parent = new_node;

        auto p = lower_bound(cur->parent->key.begin(), cur->parent->key.end(),
                             new_key) -
                 cur->parent->key.begin();
        cur->parent->key.insert(cur->parent->key.begin() + p, new_key);
        cur->parent->child.insert(cur->parent->child.begin() + p + 1, new_node);
      }

      overflow(cur->parent);
    }
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

  void print_info(Node *cur, uint32_t virtual_depth, vector<KEY_TYPE> &keys,
                  vector<Node *> &childs, ofstream &fout) {
    if (cur == root)
      fout << "Node: 0x0 ";
    else
      fout << "Node: " << cur << " ";
    fout << "(depth = " << virtual_depth << ")" << endl;
    fout << "keys: <0,0> ";
    for (auto k : keys) {
      uint64_t fbt_key = remove_key_index_prefix(k);
      uint32_t key_hi = fbt_key >> 32 & 0xffffffff;
      uint32_t key_lo = fbt_key & 0xffffffff;
      fout << "<" << key_hi << "," << key_lo << "> ";
    }
    fout << endl;
    fout << "child: ";
    for (auto c : childs)
      fout << c << " ";
    fout << endl;
  }
};

#endif // YCSB_C_B_PLUS_TREE_DB_H_
