/**
 * @file ShaderCache.cpp
 * @brief Implementation of the FShaderCache class for managing compiled shader caching.
 */

#include "Shader/Internal/ShaderCache.hpp"



void LumenEngine::Internal::FShaderCache::RecordHit() noexcept
{
    CacheHitCount.fetch_add( 1, std::memory_order_relaxed );
}

void LumenEngine::Internal::FShaderCache::RecordMiss() noexcept
{
    CacheMissCount.fetch_add( 1, std::memory_order_relaxed );
}

LumenEngine::UInt64 LumenEngine::Internal::FShaderCache::GetHits() const noexcept
{
    return CacheHitCount.load( std::memory_order_relaxed );
}

LumenEngine::UInt64 LumenEngine::Internal::FShaderCache::GetMisses() const noexcept
{
    return CacheMissCount.load( std::memory_order_relaxed );
}