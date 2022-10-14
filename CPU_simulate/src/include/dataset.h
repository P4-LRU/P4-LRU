#pragma once
#ifndef __DATASET_H__

#define __DATASET_H__
#include <map>
#include <unordered_map>
#include <string>
#include "defs.h"
#include "util.h"
using namespace std;

class Dataset
{
public:
    count_t TOTAL_PACKETS;
    count_t TOTAL_FLOWS;
    data_t* raw_data = NULL; 
    map<header_t, count_t> counter;

    ~Dataset()
    {
        if (raw_data)
            delete[] raw_data;
    }

    void init(string PATH, int size_per_item);
};

#endif