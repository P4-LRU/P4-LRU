#pragma once
#ifndef __DEFS_H__

#define __DEFS_H__
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <deque>
#include <cstdint>
#include <string>
#include <chrono>
#include <cmath>
#include "logger.h"

#define NEW_FILE_PERM (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

struct __header_t
{
    uint32_t srcip;
    uint32_t destip;
    uint16_t srcport;
    uint16_t destport;
    u_char protocal;
    
    bool operator<(const __header_t& tp) const
    {
        if (srcip!=tp.srcip)
            return srcip<tp.srcip;
        else if (destip!=tp.destip)
            return destip<tp.destip;
        else if (srcport!=tp.srcport)
            return srcport<tp.srcport;
        else if (destport!=tp.destport)
            return destport<tp.destport;
        else
            return protocal<tp.protocal;
    }
};

typedef __header_t header_t;

typedef struct
{
    header_t id;
    int size;
    double timestamp;
} data_t;

typedef uint32_t fp_t;
typedef uint64_t count_t;
typedef uint64_t seed_t;
typedef int32_t weight_t;

typedef std::chrono::high_resolution_clock::time_point TP;

#endif