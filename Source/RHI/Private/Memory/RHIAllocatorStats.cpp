/**
 * @file RHIAllocatorStats.cpp
 * @brief Implements allocator statistics transitions.
 */

#include <RHI/Memory/RHIAllocatorStats.hpp>

void Lumen::RHI::FRHIAllocatorStats::OnAllocate ( const UInt64 InSize ) noexcept
{
    Used += InSize;
    Peak = Used > Peak ? Used : Peak;
    ++AllocationCount;
}

void Lumen::RHI::FRHIAllocatorStats::OnFree ( const UInt64 InSize ) noexcept
{
    Used = InSize <= Used ? Used - InSize : 0ULL;
    ++FreeCount;
}

Lumen::Float64 Lumen::RHI::FRHIAllocatorStats::GetUtilization () const noexcept
{
    return Capacity == 0ULL ? 0.0 : static_cast<Float64>( Used ) / static_cast<Float64>( Capacity );
}
