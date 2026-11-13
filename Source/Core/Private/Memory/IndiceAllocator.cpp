/**
 * @file IndiceAllocator.cpp
 * @brief Implementation of FIndiceAllocator and FIndiceHeapAllocator.
 */

#include <Memory/IndiceAllocator.hpp>

#include <HAL/LockGuard.hpp>
#include <HAL/PlatformMutex.hpp>

#include <bit>
#include <cassert>

/**
 * Bitmap helpers.
 */

namespace
{

/**
 * @return Number of consecutive trailing one bits.
 * @note Returns a value in the range [0, 64].
 */
LUMEN_FORCE_INLINE Lumen::UInt32 CountTrailingOnes ( Lumen::UInt64 InWord ) noexcept
{
    return static_cast<Lumen::UInt32>( std::countr_one( InWord ) );
}

/**
 * @return A mask containing InCount consecutive one bits, starting at bit zero.
 */
LUMEN_FORCE_INLINE Lumen::UInt64 NOnes ( Lumen::UInt32 InCount ) noexcept
{
    if ( InCount >= Lumen::FIndiceHeapAllocator::BitsPerWord )
    {
        return ~0ULL;
    }

    return ( ( 1ULL << InCount ) - 1ULL );
}

} // namespace

/**
 * FIndiceAllocator
 */

Lumen::TOptional<Lumen::FIndiceAllocation> Lumen::FIndiceAllocator::Allocate ( const UInt32 InNumDescriptors ) noexcept
{
    if ( InNumDescriptors == 0U or InNumDescriptors > Capacity )
    {
        return {};
    }

    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );

    /** INFO: First-fit search through the sorted free ranges. */
    USize RangeIndex = 0U;

    for ( ; RangeIndex < Ranges.size(); ++RangeIndex )
    {
        const FIndiceAllocatorRange &Range = Ranges[RangeIndex];

        const UInt32 AvailableCount = Range.Last - Range.First + 1U;

        if ( AvailableCount >= InNumDescriptors )
        {
            break;
        }
    }

    if ( RangeIndex == Ranges.size() )
    {
        return {};
    }

    FIndiceAllocatorRange &Range = Ranges[RangeIndex];
    FIndiceAllocation Allocation = { Range.First, InNumDescriptors };

    Range.First += InNumDescriptors;

    /** INFO: Remove the range if it has been completely consumed. */
    if ( Range.First > Range.Last )
    {
        Ranges.erase( Ranges.begin() + static_cast<ISize>( RangeIndex ) );
    }

    return TOptional<FIndiceAllocation>{ Allocation };
}

void Lumen::FIndiceAllocator::Free ( FIndiceAllocation InAllocation ) noexcept
{
    if ( InAllocation.Count == 0U )
    {
        return;
    }

    /** INFO: Validate the range before performing any arithmetic that could overflow. */
    if ( InAllocation.Start >= Capacity or InAllocation.Count > Capacity - InAllocation.Start ) [[unlikely]]
    {
        return;
    }

    const UInt32 FreeFirst = InAllocation.Start;
    const UInt32 FreeLast  = InAllocation.Start + InAllocation.Count - 1U;

    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );

    /**
     * INFO: Locate the first free range that is not strictly before the released range.
     *       Existing ranges are sorted and non-overlapping.
     */
    USize MergeStart = 0U;

    while ( MergeStart < Ranges.size() and Ranges[MergeStart].Last + 1U < FreeFirst )
    {
        ++MergeStart;
    }

    UInt32 MergedFirst = FreeFirst;
    UInt32 MergedLast  = FreeLast;

    /**
     * INFO: Merge all overlapping or adjacent ranges.
     *       This also keeps the free-list invariant if an overlapping range is encountered.
     */
    USize MergeEnd = MergeStart;

    while ( MergeEnd < Ranges.size() and Ranges[MergeEnd].First <= MergedLast + 1U )
    {
        const FIndiceAllocatorRange &Range = Ranges[MergeEnd];

        MergedFirst = Range.First < MergedFirst ? Range.First : MergedFirst;
        MergedLast  = Range.Last > MergedLast ? Range.Last : MergedLast;

        ++MergeEnd;
    }

    /** INFO: No adjacent or overlapping range: insert at the sorted position. */
    if ( MergeStart == MergeEnd )
    {
        Ranges.insert( Ranges.begin() + static_cast<ISize>( MergeStart ), FIndiceAllocatorRange( MergedFirst, MergedLast ) );

        return;
    }

    /** INFO: Replace the merged ranges with their union. */
    Ranges[MergeStart].First = MergedFirst;
    Ranges[MergeStart].Last  = MergedLast;

    /** INFO: Remove the additional ranges absorbed by the merge. */
    if ( MergeEnd > MergeStart + 1U )
    {
        Ranges.erase( Ranges.begin() + static_cast<ISize>( MergeStart ) + 1U, Ranges.begin() + static_cast<ISize>( MergeEnd ) );
    }
}

Lumen::TOptional<Lumen::FIndiceAllocatorRange> Lumen::FIndiceAllocator::TryGetAllocatedRange () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );

    if ( Capacity == 0U )
    {
        return {};
    }

    /** INFO: No allocated indices: the entire capacity is free. */
    if ( Ranges.size() == 1U and Ranges[0].First == 0U and Ranges[0].Last == Capacity - 1U )
    {
        return {};
    }

    /**
     * INFO: Determine the first and last allocated indices using the
     *       leading and trailing free ranges.
     */
    const UInt32 LowerBound = ( !Ranges.empty() and Ranges.front().First == 0U ) ? Ranges.front().Last + 1U : 0U;

    const UInt32 UpperBound =
        ( !Ranges.empty() and Ranges.back().Last == Capacity - 1U ) ? Ranges.back().First - 1U : Capacity - 1U;

    if ( LowerBound > UpperBound )
    {
        return {};
    }

    return TOptional<FIndiceAllocatorRange>( FIndiceAllocatorRange( LowerBound, UpperBound ) );
}

/**
 * FIndiceHeapAllocator
 */

Lumen::TOptional<Lumen::FIndiceAllocation> Lumen::FIndiceHeapAllocator::Allocate ( const UInt32 InNumIndices ) noexcept
{
    if ( InNumIndices == 0U or InNumIndices > Capacity )
    {
        return {};
    }

    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );

    /** INFO: Avoid bitmap traversal when there is insufficient free capacity. */
    if ( InNumIndices > Capacity - Allocated )
    {
        return {};
    }

    const UInt32 NumWords = static_cast<UInt32>( Bitmap.size() );

    /** INFO: The heap is completely allocated. */
    if ( FirstHint >= NumWords )
    {
        return {};
    }

    /**
     * Fast path: single-index allocation.
     *
     * INFO: Find the first free bit directly instead of searching for a run.
     */
    if ( InNumIndices == 1U )
    {
        for ( UInt32 WordIndex = FirstHint; WordIndex < NumWords; ++WordIndex )
        {
            const UInt64 FreeBits = ~Bitmap[WordIndex];

            if ( FreeBits == 0ULL )
            {
                continue;
            }

            const UInt32 BitIndex = static_cast<UInt32>( std::countr_zero( FreeBits ) );
            const UInt32 Index    = ( WordIndex * BitsPerWord ) + BitIndex;

            /** INFO: Invalid tail bits are marked as allocated during construction. */
            if ( Index >= Capacity )
            {
                continue;
            }

            Bitmap[WordIndex] |= ( 1ULL << BitIndex );

            ++Allocated;

            /** INFO: Advance to the first word that still contains free bits. */
            while ( FirstHint < NumWords and Bitmap[FirstHint] == ~0ULL )
            {
                ++FirstHint;
            }

            return TOptional<FIndiceAllocation>( FIndiceAllocation( Index, 1U ) );
        }

        return {};
    }

    /**
     * General path: contiguous multi-index allocation.
     *
     * INFO: Scan each free sequence instead of considering only the first
     *       free sequence in each word.
     *
     *       RunLength is preserved across word boundaries when the previous
     *       word ends with free bits and the current word begins with free bits.
     */
    UInt32 RunStart  = 0U;
    UInt32 RunLength = 0U;

    UInt32 AllocationStart = 0U;

    Bool Found = false;

    for ( UInt32 WordIndex = FirstHint; WordIndex < NumWords; ++WordIndex )
    {
        UInt64 FreeBits = ~Bitmap[WordIndex];

        /** INFO: An entirely allocated word terminates any pending free sequence. */
        if ( FreeBits == 0ULL )
        {
            RunLength = 0U;
            continue;
        }

        const UInt32 WordBase = WordIndex * BitsPerWord;

        while ( FreeBits != 0ULL )
        {
            /**
             * INFO: Locate the next free sequence within the current word.
             */
            const UInt32 BitOffset = static_cast<UInt32>( std::countr_zero( FreeBits ) );

            const UInt32 Consecutive = CountTrailingOnes( FreeBits >> BitOffset );

            /**
             * INFO: Extend the previous sequence if this sequence begins
             *       at bit zero and therefore crosses a word boundary.
             */
            if ( BitOffset == 0U and RunLength != 0U )
            {
                RunLength += Consecutive;
            }
            else
            {
                RunStart  = WordBase + BitOffset;
                RunLength = Consecutive;
            }

            /** INFO: A sufficiently large contiguous range has been found. */
            if ( RunLength >= InNumIndices )
            {
                AllocationStart = RunStart;
                Found           = true;
                break;
            }

            const UInt32 NextBit = BitOffset + Consecutive;

            /**
             * INFO: Preserve the trailing sequence for the next word.
             *       The next word can extend this sequence if its bit zero is free.
             */
            if ( NextBit >= BitsPerWord )
            {
                break;
            }

            /** INFO: The sequence ended before the word boundary. */
            RunLength = 0U;

            /**
             * INFO: Skip the current sequence and continue searching
             *       for another free sequence in the same word.
             */
            FreeBits &= ( ~0ULL << NextBit );
        }

        if ( Found )
        {
            break;
        }
    }

    if ( !Found )
    {
        return {};
    }

    /**
     * INFO: Commit the allocation.
     *
     *       The search has already confirmed that the entire range is free.
     *       Set the required bits word by word.
     */
    UInt32 Remaining = InNumIndices;
    UInt32 Position  = AllocationStart;

    while ( Remaining > 0U )
    {
        const UInt32 WordIndex = Position / BitsPerWord;
        const UInt32 BitIndex  = Position % BitsPerWord;

        const UInt32 Take = Remaining < ( BitsPerWord - BitIndex ) ? Remaining : ( BitsPerWord - BitIndex );

        const UInt64 Mask = NOnes( Take ) << BitIndex;

        Bitmap[WordIndex] |= Mask;

        Position += Take;
        Remaining -= Take;
    }

    Allocated += InNumIndices;

    /** INFO: Advance FirstHint past all completely allocated words. */
    while ( FirstHint < NumWords and Bitmap[FirstHint] == ~0ULL )
    {
        ++FirstHint;
    }

    return TOptional<FIndiceAllocation>( FIndiceAllocation( AllocationStart, InNumIndices ) );
}

void Lumen::FIndiceHeapAllocator::Free ( FIndiceAllocation InAllocation ) noexcept
{
    if ( InAllocation.Count == 0U )
    {
        return;
    }

    /** INFO: Validate the range before performing any arithmetic that could overflow. */
    if ( InAllocation.Start >= Capacity or InAllocation.Count > Capacity - InAllocation.Start ) [[unlikely]]
    {
        return;
    }

    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );

    /**
     * INFO: Free requires a valid, currently allocated range.
     *       This assertion catches incorrect allocation ownership in debug builds.
     */
    assert( Allocated >= InAllocation.Count );

    const UInt32 FreedWord = InAllocation.Start / BitsPerWord;

    /**
     * Fast path: single-index release.
     */
    if ( InAllocation.Count == 1U )
    {
        const UInt32 BitIndex = InAllocation.Start % BitsPerWord;
        const UInt64 Mask     = 1ULL << BitIndex;

        assert( ( Bitmap[FreedWord] & Mask ) != 0ULL );

        Bitmap[FreedWord] &= ~Mask;

        --Allocated;

        if ( FreedWord < FirstHint )
        {
            FirstHint = FreedWord;
        }

        return;
    }

    /**
     * General path: release a contiguous range.
     */
    UInt32 Remaining = InAllocation.Count;
    UInt32 Position  = InAllocation.Start;

    while ( Remaining > 0U )
    {
        const UInt32 WordIndex = Position / BitsPerWord;
        const UInt32 BitIndex  = Position % BitsPerWord;

        const UInt32 Take = Remaining < ( BitsPerWord - BitIndex ) ? Remaining : ( BitsPerWord - BitIndex );

        const UInt64 Mask = NOnes( Take ) << BitIndex;

        assert( ( Bitmap[WordIndex] & Mask ) == Mask );

        Bitmap[WordIndex] &= ~Mask;

        Position += Take;
        Remaining -= Take;
    }

    Allocated -= InAllocation.Count;

    /** INFO: The released range may introduce free bits before the current hint. */
    if ( FreedWord < FirstHint )
    {
        FirstHint = FreedWord;
    }
}

Lumen::UInt32 Lumen::FIndiceHeapAllocator::GetAllocatedCount () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );

    return Allocated;
}
