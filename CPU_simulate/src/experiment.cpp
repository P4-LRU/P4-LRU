#include <set>
#include <string>
#include <vector>
#include "experiment.h"
#include "dataset.h"
#include "sketch.h"
#include "lru.h"
#include "app1.h"
#include "app1-ideal.h"
#include "app2.h"
#include "app2-ideal.h"
using namespace std;

void test_app1()
{
    Dataset stream;
    stream.init("dataset/data.dat", 23);
    double dt = 1e-4;

    LOG_INFO("Delta T = %lf", dt);
    APP1IDEAL::init(dt);
    for (int i=0;i<stream.TOTAL_PACKETS;i++)
    {
        APP1IDEAL::insert(stream.raw_data[i]);
    }
    APP1IDEAL::result();
}

void test_app2()
{
    Dataset stream;
    stream.init("dataset/data.dat", 23);
    double dt = 1e-4;

    LOG_INFO("Delta T = %lf", dt);
    APP2::init(dt);

    for (int i=0;i<stream.TOTAL_PACKETS;i++)
    {
        APP2::insert(stream.raw_data[i]);
    }
    APP2::result();
    APP2::clear();
}

void test_app3()
{
    const count_t THRESHOLD = 1000;
    const double WINDOW_LEN = 0.01;
    int curwin = 0;

    Dataset stream;
    stream.init("dataset/data.dat", 23);
    TowerSketch front;
    LRU::init();
    TowerSketch end;
    LOG_INFO("Threshold: %llu, window size: %lfs", THRESHOLD, WINDOW_LEN);
    set<header_t> inserted_flow;
    long n_inserted_packets = 0;

    for (int i=0; i<stream.TOTAL_PACKETS; i++)
    {
        if (int(stream.raw_data[i].timestamp / WINDOW_LEN) > curwin)
        {
            front.clear();
            end.clear();
            curwin = stream.raw_data[i].timestamp / WINDOW_LEN;
        }

        if (front.insert_then_query(stream.raw_data[i]) > THRESHOLD)
        {
            n_inserted_packets++;
            inserted_flow.insert(stream.raw_data[i].id);
            LRU::slot_t victim = LRU::insert(stream.raw_data[i]);
            if (victim.cnt != 0)
            {
                count_t victim_sz = end.insert_then_query(victim.fp, victim.cnt);
                if (victim_sz > 0)
                {
                    LRU::upload += 8;
                    LRU::send_to_cpu(victim.fp, victim.cnt);
                }
            }
        }
    }

    double rst = 0;
    double sm = 0, tt = 0;
    for (auto& it : stream.counter)
    {
        double cur = LRU::query(it.first);
        rst += (it.second-cur)/it.second;
        sm += cur;
        tt += it.second;
    }
    rst /= stream.TOTAL_FLOWS;
    LOG_INFO("Total inserted flows: %lu", inserted_flow.size());
    LOG_RESULT("ARE: %lf", rst);
    LOG_RESULT("Report %lf of total %lf length (%lf)", sm, tt, sm/tt);
    LOG_RESULT("Upload: %d", LRU::upload);
    LOG_RESULT("Hit rate: %lf", double(LRU::nhit)/n_inserted_packets);
}
