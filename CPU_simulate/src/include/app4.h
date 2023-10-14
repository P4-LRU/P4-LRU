#pragma once
#ifndef __APP4_H__

#define __APP4_H__
#include "defs.h"
#include "util.h"
#include "logger.h"
#include "dataset.h"

namespace APP4_LRU
{
    void init(double dT = 2e-5);

    void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP4

namespace APP4_ELASTIC
{
    void init(double dT = 2e-5);

    void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP4

namespace APP4_COCO
{
    void init(double dT = 2e-5);

    void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP4

namespace APP4_TS
{
    void init(double dT = 2e-5);

    void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP4
#endif
