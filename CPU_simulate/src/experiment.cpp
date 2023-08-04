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
#include "app4.h"
#include "app5.h"
#include "app6.h"
using namespace std;

void test_app1()
{
    Dataset stream;
    stream.init("dataset/data.dat", 23);
    double dt = 1e-4;

    for(int j = 1; j <= 10; ++j) {
        LOG_INFO("MEM J = %d", j);
        APP1IDEAL::init(30000*j, 0);
        APP1::init(30000*j, 3, 0);
        for (int i=0;i<stream.TOTAL_PACKETS;i++)
        {
            APP1IDEAL::insert(stream.raw_data[i]);
            APP1::insert(stream.raw_data[i]);
        }
        APP1IDEAL::result();
        APP1::result();
    }

    for(int j = 0; j <= 15; ++j) {
        LOG_INFO("Delta J = %lf", 2e-5*j);
        APP1IDEAL::init(300000, 2e-5*j);
        APP1::init(300000, 3, 2e-5*j);
        for (int i=0;i<stream.TOTAL_PACKETS;i++)
        {
            APP1IDEAL::insert(stream.raw_data[i]);
            APP1::insert(stream.raw_data[i]);
        }
        APP1IDEAL::result();
        APP1::result();
    }

}

void test_app2()
{
    Dataset stream;
    stream.init("dataset/data.dat", 23);
    double dt = 1e-4;

    for(int j = 1; j <= 10; ++j) {
        LOG_INFO("MEM J = %d", j);
        APP2IDEAL::init(30000*j, 0);
        APP2::init(30000*j,4, 3, 0);
        for (int i=0;i<stream.TOTAL_PACKETS;i++)
        {
            APP2IDEAL::insert(stream.raw_data[i]);
            APP2::insert(stream.raw_data[i]);
        }
        APP2IDEAL::result();
        APP2::result();
    }

    for(int j = 0; j <= 15; ++j) {
        LOG_INFO("Delta J = %lf", 2e-5*j);
        APP2IDEAL::init(300000, 2e-5*j);
        APP2::init(300000,4, 3, 2e-5*j);
        for (int i=0;i<stream.TOTAL_PACKETS;i++)
        {
            APP2IDEAL::insert(stream.raw_data[i]);
            APP2::insert(stream.raw_data[i]);
        }
        APP2IDEAL::result();
        APP2::result();
    }
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


void test_app4() {
    Dataset stream;
    stream.init("dataset/data.dat", 23);
    for(int i = 0; i <= 15; ++i) {
        double dt = 2e-5*i;

        LOG_INFO("Delta T = %lf", dt);
        APP4_LRU::init(dt);
        APP4_ELASTIC::init(dt);
        APP4_COCO::init(dt);
        APP4_TS::init(dt);
        for (int i=0;i<stream.TOTAL_PACKETS;i++)
        {
            APP4_LRU::insert(stream.raw_data[i]);
            APP4_ELASTIC::insert(stream.raw_data[i]);
            APP4_COCO::insert(stream.raw_data[i]);
            APP4_TS::insert(stream.raw_data[i]);
        }
        APP4_LRU::result();
        APP4_ELASTIC::result();
        APP4_COCO::result();
        APP4_TS::result();
    }

    for(int i = 1; i <= 10; ++i) {
        LOG_INFO("i = %d", i);
        APP4_LRU::init((int)(i*60000/68*64));
        APP4_ELASTIC::init((int)(i*60000*0.5));
        APP4_COCO::init((int)(i*60000*2/3));
        APP4_TS::init((int)(i*60000*2/3));
        for (int i=0;i<stream.TOTAL_PACKETS;i++)
        {
            APP4_LRU::insert(stream.raw_data[i]);
            APP4_ELASTIC::insert(stream.raw_data[i]);
            APP4_COCO::insert(stream.raw_data[i]);
            APP4_TS::insert(stream.raw_data[i]);
        }
        APP4_LRU::result();
        APP4_ELASTIC::result();
        APP4_COCO::result();
        APP4_TS::result();
    }
}

void test_app5() {
    Dataset stream;
    stream.init("dataset/data.dat", 23);
    for(int j = 0; j <= 15; ++j) {
        double dt = 2e-5*j;

        LOG_INFO("Delta T = %lf", dt);
        APP5_LRU::init(dt,5);
        APP5_ELASTIC::init(dt,5);
        APP5_COCO::init(dt,5);
        APP5_TS::init(dt,5);
        for (int i=0;i<stream.TOTAL_PACKETS;i++)
        {
            APP5_LRU::insert(stream.raw_data[i]);
            APP5_ELASTIC::insert(stream.raw_data[i]);
            APP5_COCO::insert(stream.raw_data[i]);
            APP5_TS::insert(stream.raw_data[i]);
        }
        APP5_LRU::result();
        APP5_ELASTIC::result();
        APP5_COCO::result();
        APP5_TS::result();
    }

    for(int j = 1; j <= 10; ++j) {
        LOG_INFO("i = %d", j);
        APP5_LRU::init(2e-5, j);
        APP5_ELASTIC::init(2e-5, j);
        APP5_COCO::init(2e-5,j);
        APP5_TS::init(2e-5, j);
        for (int i=0;i<stream.TOTAL_PACKETS;i++)
        {
            APP5_LRU::insert(stream.raw_data[i]);
            APP5_ELASTIC::insert(stream.raw_data[i]);
            APP5_COCO::insert(stream.raw_data[i]);
            APP5_TS::insert(stream.raw_data[i]);
        }
        APP5_LRU::result();
        APP5_ELASTIC::result();
        APP5_COCO::result();
        APP5_TS::result();
    }
}


void test_app6() {
    Dataset stream;
    stream.init("dataset/data.dat", 23);
    LOG_INFO("test mem");
    for(int j = 1; j <= 10; ++j) {
        LOG_INFO("i = %d", j);
        APP6_LRU::init(1000, j);
        APP6_ELASTIC::init(1000, j);
        APP6_COCO::init(1000, j);
        APP6_TS::init(1000, j);
        for (int i=0;i<stream.TOTAL_PACKETS;i++)
        {
            APP6_LRU::insert(stream.raw_data[i]);
            // LOG_INFO("finish LRU"); 
            APP6_ELASTIC::insert(stream.raw_data[i]);
            // LOG_INFO("finish Elastic"); 
            APP6_COCO::insert(stream.raw_data[i]);
            // LOG_INFO("finish Coco"); 
            APP6_TS::insert(stream.raw_data[i]);
            // LOG_INFO("finish TS"); 
        }
        APP6_LRU::result();
        APP6_ELASTIC::result();
        APP6_COCO::result();
        APP6_TS::result();
    }

    LOG_INFO("test threshold");
    for(int j = 1; j <= 11; ++j) {
        LOG_INFO("i = %d", j);
        APP6_LRU::init(j*100+1000, 1);
        APP6_ELASTIC::init(j*100+1000, 1);
        APP6_COCO::init(j*100+1000, 1);
        APP6_TS::init(j*100+1000, 1);
        for (int i=0;i<stream.TOTAL_PACKETS;i++)
        {
            APP6_LRU::insert(stream.raw_data[i]);
            APP6_ELASTIC::insert(stream.raw_data[i]);
            APP6_COCO::insert(stream.raw_data[i]);
            APP6_TS::insert(stream.raw_data[i]);
        }
        APP6_LRU::result();
        APP6_ELASTIC::result();
        APP6_COCO::result();
        APP6_TS::result();
    }
}
