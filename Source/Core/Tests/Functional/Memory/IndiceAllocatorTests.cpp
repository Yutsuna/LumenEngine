/**
 * @file IndiceAllocatorTests.cpp
 * @brief Functional tests for FIndiceAllocator and FIndiceHeapAllocator.
 */

#include <Memory/IndiceAllocator.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * FIndiceAllocator
 */

LUMEN_TEST( FIndiceAllocator, HandlesZeroCapacityAndZeroSizedRequests )
{
    FIndiceAllocator EmptyAllocator;

    LUMEN_CHECK_EQ( EmptyAllocator.GetCapacity(), 0U );
    LUMEN_CHECK_FALSE( EmptyAllocator.Allocate( 1U ).HasValue() );
    LUMEN_CHECK_FALSE( EmptyAllocator.Allocate( 0U ).HasValue() );
    LUMEN_CHECK_FALSE( EmptyAllocator.TryGetAllocatedRange().HasValue() );

    FIndiceAllocator Allocator( 8U );

    LUMEN_CHECK_EQ( Allocator.GetCapacity(), 8U );
    LUMEN_CHECK_FALSE( Allocator.Allocate( 0U ).HasValue() );
    LUMEN_CHECK_FALSE( Allocator.Allocate( 9U ).HasValue() );
    LUMEN_CHECK_FALSE( Allocator.TryGetAllocatedRange().HasValue() );
}

LUMEN_TEST( FIndiceAllocator, AllocatesRangesUsingFirstFit )
{
    FIndiceAllocator Allocator( 16U );

    const auto FirstAllocation = Allocator.Allocate( 4U );
    LUMEN_CHECK_HAS_VALUE( FirstAllocation );
    if ( FirstAllocation.HasValue() )
    {
        LUMEN_CHECK_EQ( FirstAllocation->Start, 0U );
        LUMEN_CHECK_EQ( FirstAllocation->Count, 4U );
    }

    const auto SecondAllocation = Allocator.Allocate( 3U );
    LUMEN_CHECK_HAS_VALUE( SecondAllocation );
    if ( SecondAllocation.HasValue() )
    {
        LUMEN_CHECK_EQ( SecondAllocation->Start, 4U );
        LUMEN_CHECK_EQ( SecondAllocation->Count, 3U );
    }

    const auto ThirdAllocation = Allocator.Allocate( 9U );
    LUMEN_CHECK_HAS_VALUE( ThirdAllocation );
    if ( ThirdAllocation.HasValue() )
    {
        LUMEN_CHECK_EQ( ThirdAllocation->Start, 7U );
        LUMEN_CHECK_EQ( ThirdAllocation->Count, 9U );
    }

    LUMEN_CHECK_FALSE( Allocator.Allocate( 1U ).HasValue() );

    const auto AllocatedRange = Allocator.TryGetAllocatedRange();
    LUMEN_CHECK_HAS_VALUE( AllocatedRange );
    if ( AllocatedRange.HasValue() )
    {
        LUMEN_CHECK_EQ( AllocatedRange->First, 0U );
        LUMEN_CHECK_EQ( AllocatedRange->Last, 15U );
    }
}

LUMEN_TEST( FIndiceAllocator, ReusesTheFirstSuitableFreedRange )
{
    FIndiceAllocator Allocator( 12U );

    const auto FirstAllocation  = Allocator.Allocate( 3U );
    const auto SecondAllocation = Allocator.Allocate( 3U );
    const auto ThirdAllocation  = Allocator.Allocate( 3U );
    const auto FourthAllocation = Allocator.Allocate( 3U );

    LUMEN_CHECK_HAS_VALUE( FirstAllocation );
    LUMEN_CHECK_HAS_VALUE( SecondAllocation );
    LUMEN_CHECK_HAS_VALUE( ThirdAllocation );
    LUMEN_CHECK_HAS_VALUE( FourthAllocation );

    if ( not FirstAllocation.HasValue() or not SecondAllocation.HasValue() or not ThirdAllocation.HasValue() or
         not FourthAllocation.HasValue() )
    {
        return;
    }

    Allocator.Free( *SecondAllocation );

    const auto ReusedAllocation = Allocator.Allocate( 2U );
    LUMEN_CHECK_HAS_VALUE( ReusedAllocation );
    if ( ReusedAllocation.HasValue() )
    {
        LUMEN_CHECK_EQ( ReusedAllocation->Start, 3U );
        LUMEN_CHECK_EQ( ReusedAllocation->Count, 2U );
    }
}

LUMEN_TEST( FIndiceAllocator, MergesFreedRangesOnBothSides )
{
    FIndiceAllocator Allocator( 12U );

    const auto First  = Allocator.Allocate( 3U );
    const auto Second = Allocator.Allocate( 3U );
    const auto Third  = Allocator.Allocate( 3U );
    const auto Fourth = Allocator.Allocate( 3U );

    LUMEN_CHECK_HAS_VALUE( First );
    LUMEN_CHECK_HAS_VALUE( Second );
    LUMEN_CHECK_HAS_VALUE( Third );
    LUMEN_CHECK_HAS_VALUE( Fourth );

    if ( not First.HasValue() or not Second.HasValue() or not Third.HasValue() or not Fourth.HasValue() )
    {
        return;
    }

    /** INFO: Create two free ranges separated by the third allocation. */
    Allocator.Free( *Second );
    Allocator.Free( *Fourth );

    /** INFO: Freeing the third range must bridge and merge both free ranges. */
    Allocator.Free( *Third );

    const auto MergedAllocation = Allocator.Allocate( 9U );

    LUMEN_CHECK_HAS_VALUE( MergedAllocation );

    if ( not MergedAllocation.HasValue() )
    {
        return;
    }

    LUMEN_CHECK_EQ( MergedAllocation->Start, 3U );
    LUMEN_CHECK_EQ( MergedAllocation->Count, 9U );

    /** INFO: Only the merged allocation remains allocated. */
    Allocator.Free( *First );

    const auto RemainingRange = Allocator.TryGetAllocatedRange();

    LUMEN_CHECK_HAS_VALUE( RemainingRange );

    if ( RemainingRange.HasValue() )
    {
        LUMEN_CHECK_EQ( RemainingRange->First, 3U );
        LUMEN_CHECK_EQ( RemainingRange->Last, 11U );
    }

    /** INFO: Release the final allocation. */
    Allocator.Free( *MergedAllocation );

    LUMEN_CHECK_FALSE( Allocator.TryGetAllocatedRange().HasValue() );
}

LUMEN_TEST( FIndiceAllocator, MergesWithOnlyThePreviousOrNextFreeRange )
{
    /** INFO: Merge with the next range only. */
    FIndiceAllocator NextRangeAllocator( 8U );

    const auto NextRangeAllocation = NextRangeAllocator.Allocate( 4U );
    LUMEN_CHECK_HAS_VALUE( NextRangeAllocation );
    if ( not NextRangeAllocation.HasValue() )
    {
        return;
    }

    NextRangeAllocator.Free( *NextRangeAllocation );
    LUMEN_CHECK_FALSE( NextRangeAllocator.TryGetAllocatedRange().HasValue() );

    /** INFO: Merge with the previous range only. */
    FIndiceAllocator PreviousRangeAllocator( 8U );

    const auto FirstAllocation  = PreviousRangeAllocator.Allocate( 4U );
    const auto SecondAllocation = PreviousRangeAllocator.Allocate( 4U );

    LUMEN_CHECK_HAS_VALUE( FirstAllocation );
    LUMEN_CHECK_HAS_VALUE( SecondAllocation );

    if ( not FirstAllocation.HasValue() or not SecondAllocation.HasValue() )
    {
        return;
    }

    PreviousRangeAllocator.Free( *FirstAllocation );
    PreviousRangeAllocator.Free( *SecondAllocation );
    LUMEN_CHECK_FALSE( PreviousRangeAllocator.TryGetAllocatedRange().HasValue() );
}

LUMEN_TEST( FIndiceAllocator, ReportsTheEnclosingAllocatedRange )
{
    FIndiceAllocator Allocator( 20U );

    const auto Prefix = Allocator.Allocate( 5U );
    const auto Middle = Allocator.Allocate( 4U );
    const auto Suffix = Allocator.Allocate( 6U );

    LUMEN_CHECK_HAS_VALUE( Prefix );
    LUMEN_CHECK_HAS_VALUE( Middle );
    LUMEN_CHECK_HAS_VALUE( Suffix );

    if ( not Prefix.HasValue() or not Middle.HasValue() or not Suffix.HasValue() )
    {
        return;
    }

    Allocator.Free( *Prefix );
    Allocator.Free( *Suffix );

    const auto AllocatedRange = Allocator.TryGetAllocatedRange();
    LUMEN_CHECK_HAS_VALUE( AllocatedRange );
    if ( AllocatedRange.HasValue() )
    {
        LUMEN_CHECK_EQ( AllocatedRange->First, 5U );
        LUMEN_CHECK_EQ( AllocatedRange->Last, 8U );
    }

    Allocator.Free( *Middle );
    LUMEN_CHECK_FALSE( Allocator.TryGetAllocatedRange().HasValue() );
}

LUMEN_TEST( FIndiceAllocator, IgnoresInvalidFreeRanges )
{
    FIndiceAllocator Allocator( 10U );

    const auto FullAllocation = Allocator.Allocate( 10U );
    LUMEN_CHECK_HAS_VALUE( FullAllocation );
    if ( not FullAllocation.HasValue() )
    {
        return;
    }

    Allocator.Free( FIndiceAllocation( 10U, 1U ) );
    Allocator.Free( FIndiceAllocation( 9U, 2U ) );
    Allocator.Free( FIndiceAllocation( 0U, 0U ) );

    const auto AllocatedRange = Allocator.TryGetAllocatedRange();
    LUMEN_CHECK_HAS_VALUE( AllocatedRange );
    if ( AllocatedRange.HasValue() )
    {
        LUMEN_CHECK_EQ( AllocatedRange->First, 0U );
        LUMEN_CHECK_EQ( AllocatedRange->Last, 9U );
    }

    Allocator.Free( *FullAllocation );
    LUMEN_CHECK_FALSE( Allocator.TryGetAllocatedRange().HasValue() );
}

/**
 * FIndiceHeapAllocator
 */

LUMEN_TEST( FIndiceHeapAllocator, HandlesZeroCapacity )
{
    FIndiceHeapAllocator Allocator;

    LUMEN_CHECK_EQ( Allocator.GetCapacity(), 0U );
    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 0U );
    LUMEN_CHECK_FALSE( Allocator.Allocate( 1U ).HasValue() );
    LUMEN_CHECK_FALSE( Allocator.Allocate( 0U ).HasValue() );
}

LUMEN_TEST( FIndiceHeapAllocator, RejectsZeroAndOversizedAllocations )
{
    FIndiceHeapAllocator Allocator( 8U );

    LUMEN_CHECK_FALSE( Allocator.Allocate( 0U ).HasValue() );
    LUMEN_CHECK_FALSE( Allocator.Allocate( 9U ).HasValue() );
    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 0U );
}

LUMEN_TEST( FIndiceHeapAllocator, AllocatesAndReleasesEveryTailCapacity )
{
    /** INFO: Exercise capacities around 64-bit word boundaries and their tail masks. */
    for ( UInt32 Capacity = 1U; Capacity <= 130U; ++Capacity )
    {
        FIndiceHeapAllocator Allocator( Capacity );

        const auto FullAllocation = Allocator.Allocate( Capacity );
        LUMEN_CHECK_HAS_VALUE( FullAllocation );

        if ( not FullAllocation.HasValue() )
        {
            continue;
        }

        LUMEN_CHECK_EQ( FullAllocation->Start, 0U );
        LUMEN_CHECK_EQ( FullAllocation->Count, Capacity );
        LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), Capacity );

        /** INFO: An additional allocation must fail without changing the count. */
        LUMEN_CHECK_FALSE( Allocator.Allocate( 1U ).HasValue() );
        LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), Capacity );

        Allocator.Free( *FullAllocation );
        LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 0U );

        const auto Reallocated = Allocator.Allocate( Capacity );
        LUMEN_CHECK_HAS_VALUE( Reallocated );
        if ( Reallocated.HasValue() )
        {
            LUMEN_CHECK_EQ( Reallocated->Start, 0U );
            LUMEN_CHECK_EQ( Reallocated->Count, Capacity );
        }
    }
}

LUMEN_TEST( FIndiceHeapAllocator, AllocatesSingleIndicesInAscendingOrder )
{
    FIndiceHeapAllocator Allocator( 130U );

    for ( UInt32 ExpectedIndex = 0U; ExpectedIndex < 130U; ++ExpectedIndex )
    {
        const auto Allocation = Allocator.Allocate( 1U );
        LUMEN_CHECK_HAS_VALUE( Allocation );

        if ( not Allocation.HasValue() )
        {
            return;
        }

        LUMEN_CHECK_EQ( Allocation->Start, ExpectedIndex );
        LUMEN_CHECK_EQ( Allocation->Count, 1U );
    }

    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 130U );
    LUMEN_CHECK_FALSE( Allocator.Allocate( 1U ).HasValue() );
}

LUMEN_TEST( FIndiceHeapAllocator, ReusesAnEarlierWordAfterTheHintAdvances )
{
    FIndiceHeapAllocator Allocator( 128U );

    const auto FirstWord  = Allocator.Allocate( 64U );
    const auto SecondWord = Allocator.Allocate( 64U );

    LUMEN_CHECK_HAS_VALUE( FirstWord );
    LUMEN_CHECK_HAS_VALUE( SecondWord );

    if ( not FirstWord.HasValue() or not SecondWord.HasValue() )
    {
        return;
    }

    LUMEN_CHECK_EQ( FirstWord->Start, 0U );
    LUMEN_CHECK_EQ( SecondWord->Start, 64U );
    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 128U );

    Allocator.Free( *FirstWord );
    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 64U );

    const auto Reused = Allocator.Allocate( 1U );
    LUMEN_CHECK_HAS_VALUE( Reused );
    if ( Reused.HasValue() )
    {
        LUMEN_CHECK_EQ( Reused->Start, 0U );
    }
}

LUMEN_TEST( FIndiceHeapAllocator, FindsLaterFreeSequencesWithinTheSameWord )
{
    FIndiceHeapAllocator Allocator( 64U );

    const auto FullAllocation = Allocator.Allocate( 64U );
    LUMEN_CHECK_HAS_VALUE( FullAllocation );
    if ( not FullAllocation.HasValue() )
    {
        return;
    }

    /** INFO: The first free sequence is too short; the second one is large enough. */
    Allocator.Free( FIndiceAllocation( 0U, 3U ) );
    Allocator.Free( FIndiceAllocation( 8U, 4U ) );

    const auto Allocation = Allocator.Allocate( 4U );
    LUMEN_CHECK_HAS_VALUE( Allocation );
    if ( Allocation.HasValue() )
    {
        LUMEN_CHECK_EQ( Allocation->Start, 8U );
        LUMEN_CHECK_EQ( Allocation->Count, 4U );
    }

    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 61U );
}

LUMEN_TEST( FIndiceHeapAllocator, FindsContiguousRunsAcrossWordBoundaries )
{
    FIndiceHeapAllocator Allocator( 130U );

    const auto FullAllocation = Allocator.Allocate( 130U );
    LUMEN_CHECK_HAS_VALUE( FullAllocation );
    if ( not FullAllocation.HasValue() )
    {
        return;
    }

    /** INFO: This free range starts in word zero and ends in word one. */
    Allocator.Free( FIndiceAllocation( 60U, 10U ) );

    const auto Allocation = Allocator.Allocate( 10U );
    LUMEN_CHECK_HAS_VALUE( Allocation );
    if ( Allocation.HasValue() )
    {
        LUMEN_CHECK_EQ( Allocation->Start, 60U );
        LUMEN_CHECK_EQ( Allocation->Count, 10U );
    }

    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 130U );
}

LUMEN_TEST( FIndiceHeapAllocator, DoesNotJoinFreeRunsSeparatedByAnAllocatedBit )
{
    FIndiceHeapAllocator Allocator( 128U );

    const auto FullAllocation = Allocator.Allocate( 128U );
    LUMEN_CHECK_HAS_VALUE( FullAllocation );
    if ( not FullAllocation.HasValue() )
    {
        return;
    }

    Allocator.Free( FIndiceAllocation( 60U, 4U ) );
    Allocator.Free( FIndiceAllocation( 65U, 4U ) );

    /** INFO: The allocated bit at index 64 separates the two four-bit runs. */
    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 120U );
    LUMEN_CHECK_FALSE( Allocator.Allocate( 5U ).HasValue() );
    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 120U );

    const auto FirstRun = Allocator.Allocate( 4U );
    LUMEN_CHECK_HAS_VALUE( FirstRun );
    if ( FirstRun.HasValue() )
    {
        LUMEN_CHECK_EQ( FirstRun->Start, 60U );
    }

    const auto SecondRun = Allocator.Allocate( 4U );
    LUMEN_CHECK_HAS_VALUE( SecondRun );
    if ( SecondRun.HasValue() )
    {
        LUMEN_CHECK_EQ( SecondRun->Start, 65U );
    }

    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 128U );
}

LUMEN_TEST( FIndiceHeapAllocator, RejectsInvalidFreeRanges )
{
    FIndiceHeapAllocator Allocator( 16U );

    const auto FullAllocation = Allocator.Allocate( 16U );
    LUMEN_CHECK_HAS_VALUE( FullAllocation );
    if ( not FullAllocation.HasValue() )
    {
        return;
    }

    Allocator.Free( FIndiceAllocation( 16U, 1U ) );
    Allocator.Free( FIndiceAllocation( 15U, 2U ) );
    Allocator.Free( FIndiceAllocation( 0U, 0U ) );

    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 16U );

    Allocator.Free( *FullAllocation );
    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 0U );
}

LUMEN_TEST( FIndiceHeapAllocator, MaintainsAllocationCountAcrossFragmentationAndReuse )
{
    FIndiceHeapAllocator Allocator( 96U );

    const auto First  = Allocator.Allocate( 24U );
    const auto Second = Allocator.Allocate( 24U );
    const auto Third  = Allocator.Allocate( 24U );
    const auto Fourth = Allocator.Allocate( 24U );

    LUMEN_CHECK_HAS_VALUE( First );
    LUMEN_CHECK_HAS_VALUE( Second );
    LUMEN_CHECK_HAS_VALUE( Third );
    LUMEN_CHECK_HAS_VALUE( Fourth );

    if ( not First.HasValue() or not Second.HasValue() or not Third.HasValue() or not Fourth.HasValue() )
    {
        return;
    }

    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 96U );

    Allocator.Free( *Second );
    Allocator.Free( *Fourth );
    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 48U );

    /** INFO: A request larger than either individual free range must fail. */
    LUMEN_CHECK_FALSE( Allocator.Allocate( 25U ).HasValue() );
    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 48U );

    const auto Reused = Allocator.Allocate( 16U );
    LUMEN_CHECK_HAS_VALUE( Reused );
    if ( Reused.HasValue() )
    {
        LUMEN_CHECK_EQ( Reused->Start, 24U );
        LUMEN_CHECK_EQ( Reused->Count, 16U );
    }

    LUMEN_CHECK_EQ( Allocator.GetAllocatedCount(), 64U );
}

} // namespace Lumen
