#include "app1.h"
#include "hash.h"
#include <queue>
#include <deque>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>

namespace APP2
{
    struct slot_t
    {
        fp_t fp;
        double timestamp;
    };
    

    struct pending_item
    {
        fp_t fp;
        int stage;
        double timestamp;

        bool operator<(const pending_item& tp) const
        {
            if (timestamp != tp.timestamp)
                return timestamp<tp.timestamp;
            else
                return fp < tp.fp;
        }
    };
    

    double DeltaT = 2e-5;
    int K = 3;
    int NSTAGE = 4;
    int LEN = 25000;

    int n_items, n_miss;
    double perc;
    int n_vic;
    seed_t* seed = NULL;
    seed_t fp_seed;
    std::deque<slot_t>** nt = NULL;
    std::queue<pending_item> pending_queue;
    __gnu_pbds::tree<pending_item, __gnu_pbds::null_type, less<pending_item>, __gnu_pbds::rb_tree_tag, __gnu_pbds::tree_order_statistics_node_update> ts_order;
    std::map<pending_item, count_t> dup_cntr;

    void init(double dT)
    {
        DeltaT = dT;
        if (!seed)
            seed = new seed_t[NSTAGE];
        for (int i=0;i<NSTAGE;i++)
        {
            seed[i] = clock();
            sleep(1);
        }
        fp_seed = clock();
        n_items = 0;
        n_miss = 0;
        perc = 0;
        n_vic = 0;

        nt = new std::deque<slot_t>*[NSTAGE];
        for (int i=0;i<NSTAGE;i++)
            nt[i] = new std::deque<slot_t>[LEN];

        pending_queue = std::queue<pending_item>();
        ts_order.clear();
        dup_cntr.clear();
    }

    void init(int TOTAL_BUCKET, int nstage)
    {
        NSTAGE = nstage;
        LEN = TOTAL_BUCKET/(K*NSTAGE);
        if (!seed)
            seed = new seed_t[NSTAGE];
        for (int i=0;i<NSTAGE;i++)
        {
            seed[i] = clock();
            sleep(1);
        }
        fp_seed = clock();
        n_items = 0;
        n_miss = 0;
        perc = 0;
        n_vic = 0;

        nt = new std::deque<slot_t>*[NSTAGE];
        for (int i=0;i<NSTAGE;i++)
            nt[i] = new std::deque<slot_t>[LEN];

        pending_queue = std::queue<pending_item>();
        ts_order.clear();
        dup_cntr.clear();
    }

    void init(int TOTAL_BUCKET, int nstage, int k, double delta)
    {
        K = k;
        NSTAGE = nstage;
        LEN = TOTAL_BUCKET/(K*NSTAGE);
        DeltaT = delta;
        if (!seed)
            seed = new seed_t[NSTAGE];
        for (int i=0;i<NSTAGE;i++)
        {
            seed[i] = clock();
            sleep(1);
        }
        fp_seed = clock();
        n_items = 0;
        n_miss = 0;
        perc = 0;
        n_vic = 0;

        nt = new std::deque<slot_t>*[NSTAGE];
        for (int i=0;i<NSTAGE;i++)
            nt[i] = new std::deque<slot_t>[LEN];

        pending_queue = std::queue<pending_item>();
        ts_order.clear();
        dup_cntr.clear();
    }



    void insert(data_t item)
    {
        assert(ts_order.size() <= K*NSTAGE*LEN);
        while (!pending_queue.empty())
        {
            pending_item cur = pending_queue.front();
            if (item.timestamp - cur.timestamp > DeltaT)
            {
                if (cur.stage == -1)
                {
                    auto dupit = dup_cntr.find(pending_item{cur.fp, 0, cur.timestamp});
                    if (dupit == dup_cntr.end())
                        dup_cntr.insert(std::make_pair(pending_item{cur.fp, 0, cur.timestamp}, 1));
                    else
                        dupit->second++;
                    ts_order.insert(pending_item{cur.fp, 0, cur.timestamp});

                    int pos = HASH::hash(cur.fp, seed[0]) % LEN;
                    nt[0][pos].push_front(slot_t{cur.fp, cur.timestamp});

                    for (int i=1;i<nt[0][pos].size();i++)
                    {
                        if (nt[0][pos][i].fp == cur.fp)
                        {
                            auto dupit = dup_cntr.find(pending_item{nt[0][pos][i].fp, 0, nt[0][pos][i].timestamp});
                            assert(dupit != dup_cntr.end());
                            dupit->second--;
                            if (dupit->second == 0)
                            {
                                dup_cntr.erase(dupit);
                                auto it = ts_order.find(pending_item{nt[0][pos][i].fp, 0, nt[0][pos][i].timestamp});
                                assert(it != ts_order.end());
                                ts_order.erase(it);
                            }

                            nt[0][pos].erase(nt[0][pos].begin()+i);
                            break;
                        } 
                    }

                    if (nt[0][pos].size() > K)
                    {
                        cur.fp = nt[0][pos].back().fp;
                        cur.timestamp = nt[0][pos].back().timestamp;
                        nt[0][pos].pop_back();

                        bool evict = true;
                        for (int i=1;i<NSTAGE;i++)
                        {
                            int pos = HASH::hash(cur.fp, seed[i]) % LEN;

                            if (nt[i][pos].size() < K)
                            {
                                nt[i][pos].push_back(slot_t{cur.fp, cur.timestamp});
                                evict = false;
                                break;
                            }
                            else
                            {
                                std::swap(nt[i][pos][K-1].fp, cur.fp);
                                std::swap(nt[i][pos][K-1].timestamp, cur.timestamp);
                            }
                        }

                        if (evict)
                        {
                            n_vic++;
                            perc += double(ts_order.order_of_key(pending_item{cur.fp, 0, cur.timestamp}))/ts_order.size();

                            auto dupit = dup_cntr.find(pending_item{cur.fp, 0, cur.timestamp});
                            if (dupit->second > 1)
                                dupit->second--;
                            else
                            {
                                dup_cntr.erase(dupit);
                                auto it = ts_order.find(pending_item{cur.fp, 0, cur.timestamp});
                                assert(it!=ts_order.end());
                                ts_order.erase(it);
                            }
                        }
                    }
                }
                else
                {
                    int pos = HASH::hash(cur.fp, seed[cur.stage]) % LEN;
                    nt[cur.stage][pos].push_front(slot_t{cur.fp, cur.timestamp});

                    auto dupit = dup_cntr.find(pending_item{cur.fp, 0, cur.timestamp});
                    if (dupit==dup_cntr.end())
                        dup_cntr.insert(std::make_pair(pending_item{cur.fp, 0, cur.timestamp}, 1));
                    else
                        dupit->second++;
                    ts_order.insert(pending_item{cur.fp, 0, cur.timestamp});

                    for (int i=1;i<nt[cur.stage][pos].size();i++)
                    {
                        if (nt[cur.stage][pos][i].fp == cur.fp)
                        {
                            auto dupit = dup_cntr.find(pending_item{nt[cur.stage][pos][i].fp, 0, nt[cur.stage][pos][i].timestamp});
                            dupit->second--;
                            if (dupit->second == 0)
                            {
                                dup_cntr.erase(dupit);
                                auto it = ts_order.find(pending_item{nt[cur.stage][pos][i].fp, 0, nt[cur.stage][pos][i].timestamp});
                                assert(it!=ts_order.end());
                                ts_order.erase(it);
                            }
                            nt[cur.stage][pos].erase(nt[cur.stage][pos].begin()+i);
                            break;
                        }
                    }
                    if (nt[cur.stage][pos].size()>K)
                    {
                        auto victim = nt[cur.stage][pos].back();
                        n_vic++;
                        perc += double(ts_order.order_of_key(pending_item{victim.fp, 0, victim.timestamp}))/ts_order.size();

                        auto dupit = dup_cntr.find(pending_item{victim.fp, 0, victim.timestamp});
                        if (dupit->second <= 1)
                        {
                            dup_cntr.erase(dupit);
                            auto it = ts_order.find(pending_item{victim.fp, 0, victim.timestamp});
                            assert(it!=ts_order.end());
                            ts_order.erase(it);
                        }
                        else
                            dupit->second--;

                        nt[cur.stage][pos].pop_back();
                    }
                }

                pending_queue.pop();
            }
            else
                break;
        }


        n_items++;
        fp_t fp = HASH::hash(item, fp_seed);
        for (int i=0;i<NSTAGE;i++)
        {
            int pos = HASH::hash(fp, seed[i]) % LEN;
            for (int j=0;j<nt[i][pos].size();j++)
            {
                if (nt[i][pos][j].fp == fp)
                {
                    pending_queue.push(pending_item{fp, i, item.timestamp});
                    return;
                }
            }
        }
        pending_queue.push(pending_item{fp, -1, item.timestamp});
        n_miss++;
    }

    void result()
    {
        LOG_RESULT("[APP #2] Miss Rate: %lf, AVG Rank: %lf", double(n_miss)/n_items, double(perc)/n_vic);
    }

    void clear()
    {
        for (int i=0;i<NSTAGE;i++)
        {
            delete[] nt[i];
        }
        delete[] nt;
    }

} // namespace APP2
