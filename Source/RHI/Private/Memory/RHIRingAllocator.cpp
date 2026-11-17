/**
 * @file RHIRingAllocator.cpp
 * @brief Implements circular range allocation with explicit retirement.
 */
#include <HAL/LockGuard.hpp>
#include <Memory/RHIRingAllocator.hpp>
#include <Utilities/RHIAlignment.hpp>
#include <Utilities/RHICheckedMath.hpp>
Lumen::FRHIRingAllocator::FRHIRingAllocator ( const UInt64 InCapacity ) noexcept : Capacity( InCapacity )
{
    Stats.Capacity = InCapacity;
}
Lumen::TExpected<Lumen::FRHIAllocation, Lumen::FRHIError>
Lumen::FRHIRingAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    UInt64 Start = Cursor;
    for ( UInt32 Attempt = 0U; Attempt < 2U; ++Attempt )
    {
        const auto Aligned = RHIAlignUp( Start, InDesc.Alignment );
        if ( not Aligned )
        {
            break;
        }
        const auto End = RHICheckedAdd( *Aligned, InDesc.Size );
        Bool bOverlaps = false;
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
                FRHIAllocation Result{ FRHIAllocationId{ NextId++ }, *Aligned, InDesc.Size, InDesc.Alignment,
                                       InDesc.MemoryClass,           nullptr };
                Active.push_back( Result );
                Cursor = *End == Capacity ? 0ULL : *End;
                Stats.OnAllocate( InDesc.Size );
                return Result;
            }
        }
        Start = 0ULL;
    }
    return TUnexpected<FRHIError>(
        FRHIError( ERHIErrorCode::OutOfMemory, ERHIErrorCondition::Memory, "Ring has no completed contiguous range" ) );
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIRingAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
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
void Lumen::FRHIRingAllocator::Reset () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Active.clear();
    Cursor     = 0ULL;
    Stats.Used = 0ULL;
}
Lumen::FRHIAllocatorStats Lumen::FRHIRingAllocator::GetStats () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Stats;
}
