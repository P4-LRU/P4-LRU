#include "app1.h"
#include "hash.h"
#include <queue>
#include <deque>
#include <algorithm>
#include <list>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>

namespace APP1
{
    struct slot_t
    {
        fp_t fp;
        bool valid;
        double timestamp;
    };

    struct pending_item
    {
        fp_t fp;
        double timestamp;

        bool operator<(const pending_item& tp) const
        {
            if (timestamp != tp.timestamp)
                return timestamp < tp.timestamp;
            else
                return fp < tp.fp;
        }
    };
    

    double DeltaT = 2e-5;
    const int K = 3;
    int NBUCKET = 100000;

    int n_items, n_miss;
    int n_hit, n_pholder;
    double perc;
    int n_vic;
    seed_t seed;
    seed_t fp_seed;
    std::deque<slot_t>* nt = NULL;
    std::queue<pending_item> pending_queue;
    __gnu_pbds::tree<pending_item, __gnu_pbds::null_type, less<pending_item>, __gnu_pbds::rb_tree_tag, __gnu_pbds::tree_order_statistics_node_update> ts_order;
    std::map<fp_t, count_t> dup_cntr;

    void init(double dT)
    {
        DeltaT = dT;
        seed = clock();
        sleep(1);
        fp_seed = clock();
        n_items = 0;
        n_miss = 0;
        perc =  0;
        n_vic = 0;
        n_hit = 0;
        n_pholder = 0;

        if (!nt)
            delete[] nt;
            
        nt = new std::deque<slot_t>[NBUCKET];
        pending_queue = std::queue<pending_item>();
        ts_order.clear();
        dup_cntr.clear();
        LOG_INFO("#Buckets: %d, dT: %lf", K*NBUCKET, DeltaT);
    }

    void init(int TOTAL_BUCKET)
    {
        NBUCKET = TOTAL_BUCKET / K;
        seed = clock();
        sleep(1);
        fp_seed = clock();
        n_items = 0;
        n_miss = 0;
        perc =  0;
        n_vic = 0;
        n_hit = 0;
        n_pholder = 0;

        if (!nt)
            delete[] nt;
            
        nt = new std::deque<slot_t>[NBUCKET];
        pending_queue = std::queue<pending_item>();
        ts_order.clear();
        dup_cntr.clear();
        LOG_INFO("#Buckets: %d, dT: %lf", K*NBUCKET, DeltaT);
    }

    void insert(data_t item)
    {
        while (!pending_queue.empty())
        {
            pending_item cur = pending_queue.front();
            if (item.timestamp - cur.timestamp > DeltaT)
            {
                int pos = HASH::hash(cur.fp, seed) % NBUCKET;
                for (int i=0;i<nt[pos].size();i++)
                {
                    if (nt[pos][i].fp == cur.fp)
                    {
                        nt[pos][i].valid = true;
                        break;
                    }
                }
                pending_queue.pop();
            }
            else
                break;
        }
        

        fp_t fp = HASH::hash(item, fp_seed);
        int cur = HASH::hash(fp, seed) % NBUCKET;
        n_items++;

        assert(ts_order.size() <= K*NBUCKET);
        nt[cur].push_front(slot_t{fp, false, item.timestamp});
        for (int i=1;i<nt[cur].size();i++)
        {
            if (nt[cur][i].fp == fp)
            {
                nt[cur][0].valid = nt[cur][i].valid;
                if (nt[cur][i].valid == false)
                {
                    n_miss++;
                    n_pholder++;
                }
                else
                {
                    n_hit++;
                }

                auto it = ts_order.find(pending_item{fp, nt[cur][i].timestamp});
                assert(it!=ts_order.end());
                ts_order.erase(it);
                ts_order.insert(pending_item{fp, item.timestamp});

                nt[cur].erase(nt[cur].begin()+i);
                return;
            }
        }
        pending_queue.push(pending_item{fp, item.timestamp});
        ts_order.insert(pending_item{fp, item.timestamp});

        auto it = dup_cntr.find(fp);
        if (it == dup_cntr.end())
            dup_cntr.insert(std::make_pair(fp, 1));
        else
            it->second++;

        n_miss++;
        if (nt[cur].size() > K)
        {
            n_vic++;
            fp_t victim = nt[cur].back().fp;
            auto tsit = ts_order.find(pending_item{victim, nt[cur].back().timestamp});
            assert(tsit!=ts_order.end());
            perc += double(ts_order.order_of_key(pending_item{victim, nt[cur].back().timestamp}))/ts_order.size();
            auto it = dup_cntr.find(victim);
            assert(it != dup_cntr.end());
            it->second--;
            if (it->second == 0)
            {
                dup_cntr.erase(it);
                ts_order.erase(tsit);
            }
            nt[cur].pop_back();
        }
    }

    void result()
    {
        LOG_RESULT("[APP #1] Hit Rate: %lf, PlaceHolder Hit Rate: %lf, Miss Rate: %lf, AVG Rank: %lf", 
            double(n_hit)/n_items, double(n_pholder)/n_items, double(n_miss)/n_items, perc/n_vic);
    }

} // namespace APP1
