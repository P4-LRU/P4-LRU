#pragma once
#ifndef __APP2_IDEAL_H__

#define __APP2_IDEAL_H__
#include "defs.h"
#include "util.h"
#include "logger.h"
#include "dataset.h"

namespace APP2IDEAL
{
    void init(double dT = 2e-5);

    void init(int TOTAL_BUCKET);
    void init(int TOTAL_BUCKET, double delta);

    void insert(data_t item);

    void result();

} // namespace APP1

#endif
