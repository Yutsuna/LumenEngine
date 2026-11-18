/**
 * @file RHITlsfAllocator.cpp
 * @brief Implements two-level segregated free-list allocation.
 */

#include "Algo/ForEach.hpp"
#include <Algo/Sort.hpp>
#include <HAL/LockGuard.hpp>

#include <Memory/RHITlsfAllocator.hpp>
#include <Utilities/RHIAlignment.hpp>
#include <Utilities/RHICheckedMath.hpp>
#include <bit>

Lumen::FRHITlsfAllocator::FRHITlsfAllocator ( const UInt64 InCapacity ) noexcept : Capacity( InCapacity )
{
    Stats.Capacity = InCapacity;
    Buckets.resize( FirstLevelCount * SecondLevelCount );
    if ( InCapacity > 0ULL )
    {
        InsertFreeRange( { .Offset=0ULL, .Size=InCapacity } );
    }
}
void Lumen::FRHITlsfAllocator::GetBucket ( const UInt64 InSize, USize &OutFirst, USize &OutSecond ) noexcept
{
    const UInt32 Bits  = InSize == 0ULL ? 0U : static_cast<UInt32>( std::bit_width( InSize ) );
    OutFirst           = Bits < FirstLevelCount ? Bits : FirstLevelCount - 1U;
    const UInt32 Shift = OutFirst > 4U ? static_cast<UInt32>( OutFirst - 4U ) : 0U;
    OutSecond          = static_cast<USize>( ( InSize >> Shift ) & 0xFULL );
}
void Lumen::FRHITlsfAllocator::InsertFreeRange ( const FFreeRange InRange ) noexcept
{
    if ( InRange.Size == 0ULL )
    {
        return;
    }

    USize First = 0U;
    USize Second = 0U;

    GetBucket( InRange.Size, First, Second );
    Buckets[(First * SecondLevelCount) + Second].push_back( InRange );
}

Lumen::TExpected<Lumen::FRHIAllocation, Lumen::FRHIError>
Lumen::FRHITlsfAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    if ( const TExpected<void, FRHIError> Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }

    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    USize First = 0U;
    USize Second = 0U;

    GetBucket( InDesc.Size, First, Second );
    const USize StartBucket = (First * SecondLevelCount) + Second;
    for ( USize BucketIndex = StartBucket; BucketIndex < Buckets.size(); ++BucketIndex )
    {
        TVector<FFreeRange> &List = Buckets[BucketIndex];

        for ( USize RangeIndex = 0U; RangeIndex < List.size(); ++RangeIndex )
        {
            const FFreeRange Range         = List[RangeIndex];
            const TOptional<UInt64> Offset = RHIAlignUp( Range.Offset, InDesc.Alignment );

            if ( not Offset or *Offset < Range.Offset )
            {
                continue;
            }
            const UInt64 Prefix = *Offset - Range.Offset;
            if ( Prefix > Range.Size or InDesc.Size > Range.Size - Prefix )
            {
                continue;
            }
            List.erase( List.begin() + static_cast<ISize>( RangeIndex ) );

            InsertFreeRange( { .Offset=Range.Offset, .Size=Prefix } );
            InsertFreeRange( { .Offset=*Offset + InDesc.Size, .Size=Range.Size - Prefix - InDesc.Size } );

            const FRHIAllocationId Id{ NextId++ };
            ActiveAllocations.push_back( Id );
            Stats.OnAllocate( InDesc.Size );

            return FRHIAllocation{ .Id=Id, .Offset=*Offset, .Size=InDesc.Size, .Alignment=InDesc.Alignment, .MemoryClass=InDesc.MemoryClass, .Heap=nullptr };
        }
    }
    return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Memory, "TLSF allocator exhausted" ) );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHITlsfAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Bool bFound = false;

    for ( USize Index = 0U; Index < ActiveAllocations.size(); ++Index )
    {
        if ( ActiveAllocations[Index] == InAllocation.Id )
        {
            ActiveAllocations.erase( ActiveAllocations.begin() + static_cast<ISize>( Index ) );
            bFound = true;
            break;
        }
    }
    if ( not bFound )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "TLSF allocation is not active" ) );
    }
    TVector<FFreeRange> Ranges;

    Algo::ForEach( Buckets, [&Ranges]( TVector<FFreeRange> &List)
    {
        Ranges.insert( Ranges.end(), List.begin(), List.end() );
        List.clear();
    });

    Ranges.push_back( { InAllocation.Offset, InAllocation.Size } );

    Algo::Sort( Ranges, [] ( const FFreeRange &Left, const FFreeRange &Right )
    {
        return Left.Offset < Right.Offset;
    } );

    UInt64 CurrentOffset = 0ULL;
    UInt64 CurrentSize = 0ULL;

    for ( const FFreeRange &Range : Ranges )
    {
        if ( CurrentSize != 0ULL and Range.Offset <= CurrentOffset + CurrentSize )
        {
            CurrentSize = Range.Offset + Range.Size - CurrentOffset;
        }
        else
        {
            if ( CurrentSize != 0ULL )
            {
                InsertFreeRange( { .Offset=CurrentOffset, .Size=CurrentSize } );
            }
            CurrentOffset = Range.Offset;
            CurrentSize   = Range.Size;
        }
    }
    if ( CurrentSize != 0ULL )
    {
        InsertFreeRange( { .Offset=CurrentOffset, .Size=CurrentSize } );
    }

    Stats.OnFree( InAllocation.Size );
    return {};
}

void Lumen::FRHITlsfAllocator::Reset () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );

    for ( auto &List : Buckets )
    {
        List.clear();
    }
    ActiveAllocations.clear();
    if ( Capacity > 0ULL )
    {
        InsertFreeRange( { .Offset=0ULL, .Size=Capacity } );
    }
    Stats.Used = 0ULL;
}

Lumen::FRHIAllocatorStats Lumen::FRHITlsfAllocator::GetStats () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Stats;
}
