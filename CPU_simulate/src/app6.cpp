#include "app6.h"
#include "hash.h"
#include <queue>
#include <deque>
#include <algorithm>
#include <list>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
#include <stdlib.h>

const double global_ts_interval = 0.002;

namespace APP6_LRU
{
    struct slot_t
    {
        fp_t fp;
        int counter;
        slot_t():fp(0),counter(0){}
    };

    struct filter_slot
    {
        double timestamp;
        int counter;
    };
    
    
    const int K = 3;
    int NSTAGE = 1;
    int LEN = 20000;
    uint32_t total_packet = 0;
 
    uint32_t dis_packet = 0;
    const int filter_len = 1<<18;
    int threshold = 0;
    const double filter_ts_interval = global_ts_interval;

    int n_items, n_miss;
    int n_vic;
    seed_t* seed = NULL;
    seed_t fp_seed;
    seed_t filter_seed;
    std::deque<slot_t>** nt = NULL;

    std::vector<filter_slot> filter_1;

    void init(int filter_threshold, int i)
    {
        threshold = filter_threshold;
        if (!seed)
            seed = new seed_t[NSTAGE];
        else {
            delete []seed;
            seed = new seed_t[NSTAGE];
        }
        for (int i=0;i<NSTAGE;i++)
        {
            seed[i] = clock();
            sleep(1);
        }
        fp_seed = clock();
        n_items = 0;
        total_packet = 0;
 
        dis_packet = 0;

        n_miss = 0;
        n_vic = 0;
        LEN = 10000*i*64/68;
        sleep(1);
        filter_seed = clock();
        filter_1 = std::vector<filter_slot>(filter_len, filter_slot{0,0});

        nt = new std::deque<slot_t>*[NSTAGE];
        for (int i=0;i<NSTAGE;i++)
            nt[i] = new std::deque<slot_t>[LEN];

        LOG_INFO("#Buckets: %d, threshold: %d", K*NSTAGE*LEN, filter_threshold);
    }

    void insert(data_t item)
    {
        ++total_packet;
        int filter_index = HASH::hash(item, filter_seed)%filter_len;
        fp_t fp = HASH::hash(item, fp_seed);
        fp_t cur_fp = 0;
        int cur_counter = 0;
        if(item.timestamp - filter_1[filter_index].timestamp <= filter_ts_interval) {
            cur_counter = filter_1[filter_index].counter + item.size;
            filter_1[filter_index].counter += 1;
        }
        else {
            filter_1[filter_index].counter = item.size;
            filter_1[filter_index].timestamp = item.timestamp;
            cur_counter = item.size;
            return;
        }

        if(cur_counter < threshold ) {
            dis_packet += 1;
            return;
        }

        cur_counter = 1;
        cur_fp = fp;
        
        n_items++;
        for (int i=0;i<NSTAGE;i++)
        {
            int pos = HASH::hash(cur_fp, seed[i]) % LEN;
            for (int j=0;j<nt[i][pos].size();j++)
            {
                if (nt[i][pos][j].fp == cur_fp)
                {
                    cur_counter += nt[i][pos][j].counter;
                    nt[i][pos].erase(nt[i][pos].begin()+j);
                    nt[i][pos].push_front(slot_t{});
                    nt[i][pos].front().counter = cur_counter;
                    nt[i][pos].front().fp = cur_fp;
                    return;
                }
            }
            nt[i][pos].push_front(slot_t{});
            nt[i][pos].front().counter = cur_counter;
            nt[i][pos].front().fp = cur_fp;
            if(nt[i][pos].size() > K) {
                cur_fp = nt[i][pos][nt[i][pos].size() - 1].fp;
                cur_counter = nt[i][pos][nt[i][pos].size() - 1].counter;
                nt[i][pos].pop_back();
            }
            else {
                n_miss++;
                return;
            }
        }
        n_miss++;
    }

    void clear()
    {
        for (int i=0;i<NSTAGE;i++)
        {
            delete[] nt[i];
        }
        delete[] nt;
    }

    void result()
    {
       // LOG_RESULT("n_itmes: %d, n_miss: %d", n_items, n_miss);
        LOG_RESULT("[APP #5 LRU] Miss Rate: %lf, upload acc: %lf", double(n_miss)/(n_items+1), double(total_packet-dis_packet)/(total_packet+1));
        clear();
    }

} // namespace APP6


namespace APP6_ELASTIC
{
    struct slot_t
    {
        fp_t fp;
        uint32_t vote_p;
        uint32_t vote_n;
        uint32_t counter;

        slot_t():fp(0),vote_p(0),vote_n(0),counter(0){}
    };

    struct filter_slot
    {
        double timestamp;
        int counter;
    };
    
    
    const int K = 1;
    int NBUCKET = 60000*2/3;
    const int filter_len = 1<<18;
    uint32_t total_packet = 0;
 
    uint32_t dis_packet = 0;
    int threshold = 0;
    int replace_threshold = 8;
    const double filter_ts_interval = global_ts_interval;

    int n_items, n_miss;
    double perc;
    int n_vic;
    seed_t seed;
    seed_t fp_seed;
    seed_t filter_seed;
    slot_t* nt = NULL;

    std::vector<filter_slot> filter_1;

    void init(int filter_threshold, int i)
    {
        threshold = filter_threshold;
        seed = clock();
        sleep(1);
        fp_seed = clock();
        n_items = 0;
        total_packet = 0;
 
        dis_packet = 0;

        n_miss = 0;
        NBUCKET = (int)30000*2/3*i;
        sleep(1);
        filter_seed = clock();
        filter_1 = std::vector<filter_slot>(filter_len, filter_slot{0,0});

        if (!nt)
            delete[] nt;
            
        nt = new slot_t[NBUCKET];
        LOG_INFO("#Buckets: %d, threshold: %d", NBUCKET, filter_threshold);
    }

    void insert(data_t item)
    {
        ++total_packet;
        int filter_index = HASH::hash(item, filter_seed)%filter_len;
        fp_t fp = HASH::hash(item, fp_seed);
        fp_t cur_fp = 0;
        int cur_counter = 0;
        if(item.timestamp - filter_1[filter_index].timestamp <= filter_ts_interval) {
            cur_counter = filter_1[filter_index].counter + item.size;
            filter_1[filter_index].counter += 1;
        }
        else {
            filter_1[filter_index].counter = item.size;
            filter_1[filter_index].timestamp = item.timestamp;
            cur_counter = item.size;
            return;
        }

        if(cur_counter < threshold ) {
            dis_packet += 1;
            return;
        }

        cur_counter = 1;
        cur_fp = fp;

        n_items++;
        int cur = HASH::hash(cur_fp, seed) % NBUCKET;
        if(nt[cur].fp == cur_fp) {
            nt[cur].vote_p += 1;
            return;
        }
        else {
            nt[cur].vote_n += cur_counter;
            if(nt[cur].fp == 0 || nt[cur].vote_p == 0) {
                nt[cur].fp = cur_fp;
                nt[cur].vote_n = 0;
                nt[cur].vote_p = 1;
            }
            else if(nt[cur].fp != cur_fp && nt[cur].vote_n/nt[cur].vote_p >= replace_threshold) {
                nt[cur].fp = cur_fp;
                nt[cur].vote_n = 1;
                nt[cur].vote_p = 1;
            }
        }
        n_miss++;
    }

    void result()
    {
       // LOG_RESULT("n_itmes: %d, n_miss: %d", n_items, n_miss);
        LOG_RESULT("[APP #5 ELASTIC] Miss Rate: %lf, upload acc: %lf", double(n_miss)/(n_items+1), double(total_packet-dis_packet)/(total_packet+1));
    }

} // namespace APP6


namespace APP6_COCO
{
    struct slot_t
    {
        fp_t fp;
        uint32_t freq;
        int counter;
        slot_t():fp(0),freq(0),counter(0){}
    };

    struct filter_slot
    {
        double timestamp;
        int counter;
    };
    
    
    const int K = 1;
    int NBUCKET = 60000*2/3;
    uint32_t total_packet = 0;
 
    uint32_t dis_packet = 0;
    int threshold = 0;
    int replace_threshold = 8;
    const int filter_len = 1<<18;
    const double filter_ts_interval = global_ts_interval;

    int n_items, n_miss;
    double perc;
    int n_vic;
    seed_t seed;
    seed_t fp_seed;
    seed_t filter_seed;
    slot_t* nt = NULL;

    std::vector<filter_slot> filter_1;

    void init(int filter_threshold, int i)
    {
        threshold = filter_threshold;
        seed = clock();
        sleep(1);
        fp_seed = clock();
        n_items = 0;
        total_packet = 0;
 
        dis_packet = 0;

        n_miss = 0;
        perc = 0;
        n_vic = 0;
        NBUCKET = (int)30000*2/3*i;
        sleep(1);
        filter_seed = clock();
        filter_1 = std::vector<filter_slot>(filter_len, filter_slot{0,0});

        if (!nt)
            delete[] nt;
            
        nt = new slot_t[NBUCKET];
        LOG_INFO("#Buckets: %d, threshold: %d", NBUCKET, filter_threshold);
    }

    void insert(data_t item)
    {
        ++total_packet;
        int filter_index = HASH::hash(item, filter_seed)%filter_len;
        fp_t fp = HASH::hash(item, fp_seed);
        fp_t cur_fp = 0;
        int cur_counter = 0;
        if(item.timestamp - filter_1[filter_index].timestamp <= filter_ts_interval) {
            cur_counter = filter_1[filter_index].counter + item.size;
            filter_1[filter_index].counter += 1;
        }
        else {
            filter_1[filter_index].counter = item.size;
            filter_1[filter_index].timestamp = item.timestamp;
            cur_counter = item.size;
            return;
        }

        if(cur_counter < threshold ) {
            dis_packet += 1;
            return;
        }

        cur_counter = 1;
        cur_fp = fp;

        n_items++;
        int cur = HASH::hash(cur_fp, seed) % NBUCKET;
        nt[cur].freq++;
        if(nt[cur].fp == cur_fp) {
            nt[cur].counter += cur_counter;
            return;
        }
        else {
            unsigned int rand_seed = (unsigned int)clock();
            srand(rand_seed);
            unsigned int u_rand = rand()%nt[cur].freq;
            if(u_rand == 0) {
                nt[cur].fp = cur_fp;
                nt[cur].counter = cur_counter;
            }
        }
        n_miss++;
    }

    void result()
    {
       // LOG_RESULT("n_itmes: %d, n_miss: %d", n_items, n_miss);
        LOG_RESULT("[APP #5 COCO] Miss Rate: %lf, upload acc: %lf", double(n_miss)/(n_items+1), double(total_packet-dis_packet)/(total_packet+1));
    }

} // namespace APP6


namespace APP6_TS
{
    struct slot_t
    {
        fp_t fp;
        double timestamp;
        int counter;
        slot_t():fp(0),timestamp(0),counter(0){}
    };

    struct filter_slot
    {
        double timestamp;
        int counter;
    };
    
    
    const int K = 1;
    int NBUCKET = 60000*2/3;
    uint32_t total_packet = 0;
 
    uint32_t dis_packet = 0;
    int threshold = 0;
    double replace_threshold = 0.001;
    const int filter_len = 1<<18;
    const double filter_ts_interval = global_ts_interval;

    int n_items, n_miss;
    double perc;
    int n_vic;
    seed_t seed;
    seed_t fp_seed;
    seed_t filter_seed;
    slot_t* nt = NULL;

    std::vector<filter_slot> filter_1;

    void init(int filter_threshold, int i)
    {
        threshold = filter_threshold;
        seed = clock();
        sleep(1);
        fp_seed = clock();
        n_items = 0;
        total_packet = 0;
 
        dis_packet = 0;

        n_miss = 0;
        perc = 0;
        n_vic = 0;
        NBUCKET = (int)30000*2/3*i;
        sleep(1);
        filter_seed = clock();
        filter_1 = std::vector<filter_slot>(filter_len, filter_slot{0,0});

        if (!nt)
            delete[] nt;
            
        nt = new slot_t[NBUCKET];
        LOG_INFO("#Buckets: %d, threshold: %d", NBUCKET, filter_threshold);
    }

    void insert(data_t item)
    {
        ++total_packet;
        int filter_index = HASH::hash(item, filter_seed)%filter_len;
        fp_t fp = HASH::hash(item, fp_seed);
        fp_t cur_fp = 0;
        int cur_counter = 0;
        if(item.timestamp - filter_1[filter_index].timestamp <= filter_ts_interval) {
            cur_counter = filter_1[filter_index].counter + item.size;
            filter_1[filter_index].counter += 1;
        }
        else {
            filter_1[filter_index].counter = item.size;
            filter_1[filter_index].timestamp = item.timestamp;
            cur_counter = item.size;
            return;
        }

        if(cur_counter < threshold ) {
            dis_packet += 1;
            return;
        }

        cur_counter = 1;
        cur_fp = fp;

        n_items++;
        int cur = HASH::hash(cur_fp, seed) % NBUCKET;
        if(nt[cur].fp == cur_fp) {
            nt[cur].counter += cur_counter;
            nt[cur].timestamp = item.timestamp;
            return;
        }
        else {
            if(item.timestamp - nt[cur].timestamp >= replace_threshold) {
                nt[cur].fp = cur_fp;
                nt[cur].counter = cur_counter;
                nt[cur].timestamp = item.timestamp;
            }
        }
        n_miss++;
    }

    void result()
    {
       // LOG_RESULT("n_itmes: %d, n_miss: %d", n_items, n_miss);
        LOG_RESULT("[APP #5 TS] Miss Rate: %lf, upload acc: %lf", double(n_miss)/(n_items+1), double(total_packet-dis_packet)/(total_packet+1));
    }

} // namespace APP6
