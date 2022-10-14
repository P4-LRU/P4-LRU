#include "lru.h"
#include <map>
#include <deque>

namespace LRU
{
    const int TOTAL_SLOT = 100'000/8;
    const int NSLOT = 3;
    const int NBUCK = TOTAL_SLOT/NSLOT;
    int upload = 0;
    int nhit = 0;
    seed_t seed;
    seed_t fp_seed;
    std::deque<slot_t> nt[NBUCK];
    std::map<fp_t, count_t> in_cpu;

    void send_to_cpu(fp_t fp, count_t size)
    {
        auto it = in_cpu.find(fp);
        if (it == in_cpu.end())
        {
            in_cpu.insert(std::make_pair(fp, size));
        }
        else
        {
            it->second += size;
        }
    }

    void init()
    {
        seed=clock();
        sleep(1);
        fp_seed=clock();
        upload = 8*TOTAL_SLOT;
    }

    slot_t insert(data_t item)
    {
        fp_t fp = HASH::hash(item, fp_seed);
        int pos = HASH::hash(item, seed) % NBUCK;

        nt[pos].push_front(slot_t{fp, uint64_t(item.size)});
        for (int i=1;i<nt[pos].size();i++)
        {
            if (nt[pos][i].fp == fp)
            {
                nt[pos][0].cnt += nt[pos][i].cnt;
                nt[pos].erase(nt[pos].begin() + i);
                nhit++;
                return slot_t{0, 0};
            }
        }

        upload += 13;
        if (nt[pos].size() > NSLOT)
        {
            slot_t victim{nt[pos].back().fp, nt[pos].back().cnt};
            nt[pos].pop_back();
            return victim;
        }

        return slot_t{0, 0};
    }

    count_t query(header_t item)
    {
        fp_t fp = HASH::hash(item, fp_seed);
        int pos = HASH::hash(item, seed) % NBUCK;
        count_t rst = 0;

        for (int i=0;i<nt[pos].size();i++)
        {
            if (nt[pos][i].fp == fp)
            {
                rst += nt[pos][i].cnt;
                break;
            }
        }

        auto it = in_cpu.find(fp);
        if (it != in_cpu.end())
            rst += it->second;
        return rst;
    }
}