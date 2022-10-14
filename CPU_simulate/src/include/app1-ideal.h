#pragma once
#ifndef __APP1_IDEAL_H__

#define __APP1_IDEAL_H__
#include "defs.h"
#include "util.h"
#include "logger.h"
#include "dataset.h"

namespace APP1IDEAL
{
    void init(double dT = 2e-5);

    void init(int TOTAL_BUCKET);

    void insert(data_t item);

    void result();

} // namespace APP1

#endif
