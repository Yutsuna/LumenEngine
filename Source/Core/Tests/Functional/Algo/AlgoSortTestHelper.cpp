/**
 * @file AlgoSortTestHelper.cpp
 * @brief Shared data generators and checkers for the Algo sorting tests.
 */

#include "AlgoSortTestHelper.hpp"

#include <Tests/UnitTest.hpp>

#include <algorithm>
#include <format>
#include <functional>
#include <utility>

/**
 * Public
 */

Lumen::Tests::SortHelpers::FSortRandom::FSortRandom ( const UInt64 InSeed ) noexcept : State{ InSeed | 1ULL }
{
    /** Empty */
}

Lumen::UInt64 Lumen::Tests::SortHelpers::FSortRandom::Next () noexcept
{
    State ^= State << 13U;
    State ^= State >> 7U;
    State ^= State << 17U;
    return State;
}

/**
 * SortHelpers
 */

void Lumen::Tests::SortHelpers::FillShape ( TVector<Int32> &OutValues,
                                            const USize InCount,
                                            const ESortShape InShape,
                                            const UInt64 InSeed )
{
    FSortRandom Random( InSeed );
    OutValues.resize( InCount );

    for ( USize Index = 0; Index < InCount; ++Index )
    {
        const Int32 Position = static_cast<Int32>( Index );

        switch ( InShape )
        {
        case ESortShape::Random:
            OutValues[Index] = static_cast<Int32>( Random.Next() % 1000000ULL );
            break;
        case ESortShape::Sorted:
            OutValues[Index] = Position;
            break;
        case ESortShape::Reversed:
            OutValues[Index] = static_cast<Int32>( InCount - Index );
            break;
        case ESortShape::AllEqual:
            OutValues[Index] = 7;
            break;
        case ESortShape::FewUnique:
            OutValues[Index] = static_cast<Int32>( Random.Next() % 4ULL );
            break;
        case ESortShape::OrganPipe:
            OutValues[Index] = ( Index < InCount / 2 ) ? Position : static_cast<Int32>( InCount - Index );
            break;
        case ESortShape::SawTooth:
            OutValues[Index] = Position % 37;
            break;
        }
    }
}

void Lumen::Tests::SortHelpers::FillRecords ( TVector<FSortRecord> &OutRecords,
                                              const USize InCount,
                                              const UInt64 InKeyRange,
                                              const UInt64 InSeed )
{
    FSortRandom Random( InSeed );

    OutRecords.clear();
    OutRecords.reserve( InCount );

    for ( USize Index = 0; Index < InCount; ++Index )
    {
        const Int64 Key = static_cast<Int64>( Random.Next() % InKeyRange );
        OutRecords.push_back( FSortRecord{ .Key = Key, .Payload = Key * 3, .Extra = static_cast<Int64>( Index ) } );
    }
}

void Lumen::Tests::SortHelpers::CheckSorterOnShape ( const FIntSorter &InSorter,
                                                     const USize InCount,
                                                     const ESortShape InShape,
                                                     const Bool bDescending )
{
    TVector<Int32> Values;
    FillShape( Values, InCount, InShape, 0x9E3779B97F4A7C15ULL ^ InCount );

    TVector<Int32> Expected = Values;
    if ( bDescending )
    {
        std::ranges::sort( Expected, std::greater<>{} );
    }
    else
    {
        std::ranges::sort( Expected );
    }

    InSorter( Values );

    LUMEN_CHECK_MSG( Values == Expected, std::format( "Count={} Shape={} Descending={}", InCount, std::to_underlying( InShape ),
                                                      bDescending ) );
}

void Lumen::Tests::SortHelpers::CheckSorterOnAllShapes ( const FIntSorter &InSorter,
                                                         const Bool bDescending,
                                                         const USize InMaxCount )
{
    for ( const USize Count : SortTestSizes )
    {
        if ( Count > InMaxCount )
        {
            continue;
        }

        for ( const ESortShape Shape : AllShapes )
        {
            CheckSorterOnShape( InSorter, Count, Shape, bDescending );
        }
    }
}

void Lumen::Tests::SortHelpers::CheckSorterOnSmallSizes ( const FIntSorter &InSorter, const Bool bDescending )
{
    for ( USize Count = 0; Count <= 70; ++Count )
    {
        for ( const ESortShape Shape : AllShapes )
        {
            CheckSorterOnShape( InSorter, Count, Shape, bDescending );
        }
    }
}

void Lumen::Tests::SortHelpers::CheckStringSorter ( const FStringSorter &InSorter )
{
    FSortRandom Random( 42ULL );

    TVector<FString> Values;
    for ( USize Index = 0; Index < 500; ++Index )
    {
        Values.push_back( FString{ std::to_string( Random.Next() % 5000ULL ) } );
    }

    TVector<FString> Expected = Values;
    std::ranges::sort( Expected );

    InSorter( Values );

    LUMEN_CHECK_TRUE( Values == Expected );
}

void Lumen::Tests::SortHelpers::CheckRecordSorter ( const FRecordSorter &InSorter )
{
    constexpr USize RecordCount = 2000;

    TVector<FSortRecord> Records;
    FillRecords( Records, RecordCount, 300ULL, 99ULL );

    InSorter( Records );

    UInt64 ExtraSum = 0;
    for ( USize Index = 0; Index < RecordCount; ++Index )
    {
        LUMEN_CHECK_EQ( Records[Index].Payload, Records[Index].Key * 3 );
        if ( Index > 0 )
        {
            LUMEN_CHECK_TRUE( Records[Index - 1].Key <= Records[Index].Key );
        }
        ExtraSum += static_cast<UInt64>( Records[Index].Extra );
    }

    /** INFO: Every record must still be present exactly once. */
    LUMEN_CHECK_EQ( ExtraSum, static_cast<UInt64>( RecordCount ) * ( RecordCount - 1 ) / 2 );
}
