#pragma once
#ifndef __LRU_H__

#define __LRU_H__
#include "defs.h"
#include "util.h"
#include "hash.h"
#include "logger.h"

namespace LRU
{
    struct slot_t
    {
        fp_t fp;
        uint64_t cnt;
    };

    extern int upload;

    extern int nhit;

    void send_to_cpu(fp_t fp, count_t size);

    void init();

    slot_t insert(data_t item);

    count_t query(header_t item);
}

#endif
