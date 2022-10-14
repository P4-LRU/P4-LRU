#pragma once
#ifndef __SKETCH_H__

#define __SKETCH_H__
#include <cstring>
#include "defs.h"
#include "logger.h"
#include "hash.h"
using namespace std;

class BaseSketch
{
public:
    virtual void insert(data_t) = 0;
    virtual count_t query(header_t) = 0;
};

class CMSketch : BaseSketch
{
public:
    int nrows;
    int len;
    count_t** nt;
    seed_t* seeds;

    CMSketch(int _nrows, int _len) : nrows(_nrows), len(_len)
    {
        nt=new count_t*[nrows];
        seeds=new seed_t[nrows];
        for (int i=0;i<nrows;i++)
        {
            seeds[i]=clock();
            sleep(1);
            nt[i]=new count_t[len];
            memset(nt[i], 0, sizeof(count_t)*len);
        }
    }

    virtual void insert(data_t item) override
    {
        for (int i=0;i<nrows;i++)
        {
            int curpos = HASH::hash(item, seeds[i]) % len;
            nt[i][curpos]+=item.size;
        }
    }

    virtual count_t query(header_t item) override
    {
        count_t cnt=INT32_MAX;
        for (int i=0;i<nrows;i++)
        {
            int curpos = HASH::hash(item, seeds[i]) % len;
            cnt=min(cnt, nt[i][curpos]);
        }
        return cnt;
    }
};

class TowerSketch : public BaseSketch
{
public:
    int nrows=3;
    uint32_t* nt[3];
    seed_t seed[3];
    int len[3] = { 3'000'000, 1'500'000, 750'000 };
    uint32_t threshold[3] = { UINT8_MAX, UINT16_MAX, UINT32_MAX };

    TowerSketch()
    {
        for (int i=0;i<3;i++)
        {
            seed[i]=clock();
            sleep(1);
            nt[i]=new uint32_t[len[i]];
            memset(nt[i], 0, sizeof(uint32_t)*len[i]);
        }
    }

    void clear()
    {
        for (int i=0;i<3;i++)
        {
            memset(nt[i], 0, sizeof(uint32_t)*len[i]);
        }
    }

    virtual void insert(data_t item) override
    {
        for (int i=0;i<nrows;i++)
        {
            int pos = HASH::hash(item, seed[i]) % len[i];
            if (nt[i][pos] < threshold[i])
                nt[i][pos] += item.size;
        }
    }

    count_t insert_then_query(data_t item)
    {
        count_t val=UINT32_MAX;
        for (int i=0;i<nrows;i++)
        {
            int pos = HASH::hash(item, seed[i]) % len[i];
            if (nt[i][pos] < threshold[i])
                nt[i][pos] += item.size;
            if (nt[i][pos] < threshold[i])
                val=min(uint32_t(val), nt[i][pos]);
        }
        return val;
    }

    count_t insert_then_query(fp_t fp, count_t size)
    {
        count_t val=UINT32_MAX;
        for (int i=0;i<nrows;i++)
        {
            int pos = HASH::hash(fp, seed[i]) % len[i];
            if (nt[i][pos] < threshold[i])
                nt[i][pos] += size;
            if (nt[i][pos] < threshold[i])
                val=min(uint32_t(val), nt[i][pos]);
        }
        return val;
    }

    virtual count_t query(header_t item) override
    {
        count_t val=UINT32_MAX;
        for (int i=0;i<nrows;i++)
        {
            int pos = HASH::hash(item, seed[i]) % len[i];
            if (nt[i][pos] < threshold[i])
                val=min(uint32_t(val), nt[i][pos]);
        }
        return val;
    }
};

#endif