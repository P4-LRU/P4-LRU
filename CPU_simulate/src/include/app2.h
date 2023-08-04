#pragma once
#ifndef __APP2_H__

#define __APP2_H__
#include "defs.h"
#include "util.h"
#include "logger.h"
#include "dataset.h"

namespace APP2
{
    void init(double dT = 2e-5);

    void init(int TOTAL_BUCKET, int nstage = 4);

    void init(int TOTAL_BUCKET, int nstage, int k, double delta);

    void insert(data_t item);

    void result();

    void clear();
} // namespace APP2IDEAL

#endif
