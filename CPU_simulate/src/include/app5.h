#pragma once
#ifndef __APP5_H__

#define __APP5_H__
#include "defs.h"
#include "util.h"
#include "logger.h"
#include "dataset.h"

namespace APP5_LRU
{
    void init(double dT = 2e-5, int i = 1);

    // void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP5

namespace APP5_ELASTIC
{
    void init(double dT = 2e-5, int i = 1);

    // void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP5

namespace APP5_COCO
{
    void init(double dT = 2e-5, int i = 1);

    // void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP5

namespace APP5_TS
{
    void init(double dT = 2e-5, int i = 1);

    // void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP5

#endif
