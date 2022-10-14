#include "app1-ideal.h"
#include "hash.h"
#include <queue>
#include <deque>
#include <algorithm>
#include <list>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>

namespace APP1IDEAL
{
    struct slot_t
    {
        fp_t fp;
        double timestamp;

        bool operator<(const slot_t& tp) const
        {
            if (timestamp != tp.timestamp)
                return timestamp < tp.timestamp;
            else
                return fp < tp.fp;
        }
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
    int NBUCKET = 300000;

    int n_items, n_miss;
    int n_hit, n_pholder;
    seed_t fp_seed;
    std::map<fp_t, double> timer;
    std::map<slot_t, bool> nt;
    std::queue<pending_item> pending_queue;

    void init(double dT)
    {
        DeltaT = dT;
        sleep(1);
        fp_seed = clock();
        n_items = 0;
        n_miss = 0;
        n_hit = 0;
        n_pholder = 0;

        timer.clear();
        nt.clear();
        pending_queue = std::queue<pending_item>();
        LOG_INFO("#Buckets: %d, dT: %lf", NBUCKET, DeltaT);
    }

    void init(int TOTAL_BUCKET)
    {
        NBUCKET = TOTAL_BUCKET;
        sleep(1);
        fp_seed = clock();
        n_items = 0;
        n_miss = 0;
        n_hit = 0;
        n_pholder = 0;

        timer.clear();
        nt.clear();
        pending_queue = std::queue<pending_item>();
        LOG_INFO("#Buckets: %d, dT: %lf", NBUCKET, DeltaT);
    }

    void insert(data_t item)
    {
        while (!pending_queue.empty())
        {
            pending_item cur = pending_queue.front();
            if (item.timestamp - cur.timestamp > DeltaT)
            {
                auto tit = timer.find(cur.fp);
                if (tit != timer.end())
                {
                    auto it = nt.find(slot_t{cur.fp, tit->second});
                    assert(it!=nt.end());
                    it->second=true;
                }
                pending_queue.pop();
            }
            else
                break;
        }
        

        fp_t fp = HASH::hash(item, fp_seed);
        n_items++;

        auto tit = timer.find(fp);
        if (tit != timer.end())
        {
            auto it = nt.find(slot_t{fp, tit->second});
            assert(it!=nt.end());
            bool valid = it->second;
            if (it->second)
                n_hit++;
            else
            {
                n_miss++;
                n_pholder++;
            }
            
            tit->second = item.timestamp;
            nt.erase(it);
            nt.insert(std::make_pair(slot_t{fp, item.timestamp}, valid));
        }
        else
        {
            n_miss++;
            timer.insert(std::make_pair(fp, item.timestamp));
            nt.insert(std::make_pair(slot_t{fp, item.timestamp}, false));
            pending_queue.push(pending_item{fp, item.timestamp});
            if (nt.size() > NBUCKET)
            {
                auto it = nt.begin();
                timer.erase(timer.find(it->first.fp));
                nt.erase(it);
            }
        }
    }

    void result()
    {
        LOG_RESULT("[APP #1] Hit Rate: %lf, PlaceHolder Hit Rate: %lf, Miss Rate: %lf", 
            double(n_hit)/n_items, double(n_pholder)/n_items, double(n_miss)/n_items);
    }

} // namespace APP1IDEAL
