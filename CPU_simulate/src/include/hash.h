#pragma once
#ifndef __HASH_H__

#define __HASH_H__
#include "farm.h"
#include "defs.h"

namespace HASH
{
    inline uint64_t hash(const data_t& data, seed_t seed = 0U)
    {
        return NAMESPACE_FOR_HASH_FUNCTIONS::Hash64WithSeed(reinterpret_cast<const char *>(&data.id), 13, seed);
    }

    inline uint64_t hash(const header_t& data, seed_t seed = 0U)
    {
        return NAMESPACE_FOR_HASH_FUNCTIONS::Hash64WithSeed(reinterpret_cast<const char *>(&data), 13, seed);
    }

    inline uint64_t hash(const fp_t data, seed_t seed = 0U)
    {
        return NAMESPACE_FOR_HASH_FUNCTIONS::Hash64WithSeed(reinterpret_cast<const char *>(&data), sizeof(fp_t), seed);
    }
}

#endif