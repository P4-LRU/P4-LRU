#pragma once
#ifndef __APP6_H__

#define __APP6_H__
#include "defs.h"
#include "util.h"
#include "logger.h"
#include "dataset.h"

namespace APP6_LRU
{
    void init(int filter_threshold = 1, int i = 1);

    // void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP6

namespace APP6_ELASTIC
{
    void init(int filter_threshold = 1, int i = 1);

    // void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP6

namespace APP6_COCO
{
    void init(int filter_threshold = 1, int i = 1);

    // void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP6

namespace APP6_TS
{
    void init(int filter_threshold = 1, int i = 1);

    // void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP6

#endif
