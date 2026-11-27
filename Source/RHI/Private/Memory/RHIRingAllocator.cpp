/**
 * @file RHIRingAllocator.cpp
 * @brief Implements circular range allocation with explicit retirement.
 */

#include <HAL/LockGuard.hpp>
#include <RHI/Memory/RHIRingAllocator.hpp>

#include <RHI/Utilities/RHIAlignment.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

/**
 * Public
 */

Lumen::RHI::FRHIRingAllocator::FRHIRingAllocator ( const UInt64 InCapacity ) noexcept : Capacity( InCapacity )
{
    Stats.Capacity = InCapacity;
}

Lumen::TExpected<Lumen::RHI::FRHIAllocation, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIRingAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( _, InDesc.Validate() );

    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    UInt64 Start = Cursor;

    for ( UInt32 Attempt = 0U; Attempt < 2U; ++Attempt )
    {
        const TOptional<UInt64> Aligned = RHIAlignUp( Start, InDesc.Alignment );
        if ( not Aligned )
        {
            break;
        }

        const TOptional<UInt64> End = RHICheckedAdd( *Aligned, InDesc.Size );
        Bool bOverlaps              = false;

        if ( End and *End <= Capacity )
        {
            for ( const FRHIAllocation &Other : Active )
            {
                if ( RHIRangesOverlap( *Aligned, InDesc.Size, Other.Offset, Other.Size ) )
                {
                    bOverlaps = true;
                    break;
                }
            }
            if ( not bOverlaps )
            {
                FRHIAllocation Result{ .Id          = FRHIAllocationId{ NextId++ },
                                       .Offset      = *Aligned,
                                       .Size        = InDesc.Size,
                                       .Alignment   = InDesc.Alignment,
                                       .MemoryClass = InDesc.MemoryClass,
                                       .Heap        = nullptr };
                Active.push_back( Result );
                Cursor = *End == Capacity ? 0ULL : *End;
                Stats.OnAllocate( InDesc.Size );
                return Result;
            }
        }
        Start = 0ULL;
    }
    return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Memory, "Ring has no completed contiguous range" ) );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIRingAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );

    for ( USize Index = 0U; Index < Active.size(); ++Index )
    {
        if ( Active[Index].Id == InAllocation.Id )
        {
            Active.erase( Active.begin() + static_cast<ISize>( Index ) );
            Stats.OnFree( InAllocation.Size );
            return {};
        }
    }
    return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Ring allocation is not active" ) );
}

void Lumen::RHI::FRHIRingAllocator::Reset () noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Active.clear();
    Cursor     = 0ULL;
    Stats.Used = 0ULL;
}

Lumen::RHI::FRHIAllocatorStats Lumen::RHI::FRHIRingAllocator::GetStats () const noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Stats;
}
