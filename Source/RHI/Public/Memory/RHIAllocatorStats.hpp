/**
 * @file RHIAllocatorStats.hpp
 * @brief Defines allocator usage statistics.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

/**
 * @struct FRHIAllocatorStats
 * @brief Coherent value snapshot of allocator counters.
 */
struct RHI_EXPORT FRHIAllocatorStats
{
    UInt64 Capacity        = 0ULL;
    UInt64 Used            = 0ULL;
    UInt64 Peak            = 0ULL;
    UInt64 AllocationCount = 0ULL;
    UInt64 FreeCount       = 0ULL;
    /** @brief Updates counters after a successful allocation. */
    void OnAllocate ( UInt64 InSize ) noexcept;
    /** @brief Updates counters after a valid release. */
    void OnFree ( UInt64 InSize ) noexcept;
    /** @return Ratio in [0,1], or zero for zero capacity. */
    [[nodiscard]] Float64 GetUtilization () const noexcept;
};
} // namespace Lumen
