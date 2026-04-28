/**
 * @file CacheStats.hpp
 * @brief Cache Statistics Tracking in LumenEngine
 */

#pragma once

#include "Cache/CacheStats.hpp"

constexpr LumenEngine::Float64 LumenEngine::Cache::FCacheStats::HitRatio () const noexcept
{
    const UInt64 Total = Hits + Misses;

    return ( Total == 0ULL ) ? 0.0 : static_cast<Float64>( Hits ) / static_cast<Float64>( Total );
}
