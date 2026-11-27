/**
 * @file AllocationTracking.hpp
 * @brief Test-only global allocation counter shared by Scene allocation contracts.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <cstddef>

namespace Lumen
{

/** @brief Process-wide allocation counter; only sampled while bTrackingAllocations is set. */
extern TAtomic<UInt64> GTrackedAllocations;
extern TAtomic<Bool> bTrackingAllocations;

/** @brief Test-only allocator front end used by the single set of global operator overrides. */
[[nodiscard]] void *TrackedAllocate ( std::size_t InSize, std::size_t InAlignment ) noexcept;

/** @class FAllocationTrackingScope
 * @brief Captures an allocation baseline and exposes the delta observed until destruction.
 */
class FAllocationTrackingScope final
{
public:

    FAllocationTrackingScope () noexcept
    {
        Baseline = GTrackedAllocations.load( std::memory_order_relaxed );
        bTrackingAllocations.store( true, std::memory_order_release );
    }

    ~FAllocationTrackingScope () noexcept
    {
        bTrackingAllocations.store( false, std::memory_order_release );
    }

    [[nodiscard]] UInt64 GetCount () const noexcept
    {
        return GTrackedAllocations.load( std::memory_order_relaxed ) - Baseline;
    }

private:

    UInt64 Baseline = 0ULL;
};

} // namespace Lumen
