/**
 * @file AtomicBitsetTest.cpp
 * @brief Unit tests for the TAtomicBitset class.
 */

#include "Container/AtomicBitset.hpp"
#include "Container/Vector.hpp"
#include <gtest/gtest.h>

#include <thread>

TEST( FAtomicBitset, BasicSetClear )
{
    LumenEngine::FAtomicBitset Bitset( 128U );

    EXPECT_FALSE( Bitset.Test( 0U ) );
    EXPECT_FALSE( Bitset.Test( 63U ) );
    EXPECT_FALSE( Bitset.Test( 64U ) );
    EXPECT_FALSE( Bitset.Test( 127U ) );

    Bitset.Set( 0U );
    Bitset.Set( 64U );

    EXPECT_TRUE( Bitset.Test( 0U ) );
    EXPECT_FALSE( Bitset.Test( 63U ) );
    EXPECT_TRUE( Bitset.Test( 64U ) );
    EXPECT_FALSE( Bitset.Test( 127U ) );

    Bitset.Clear( 0U );
    EXPECT_FALSE( Bitset.Test( 0U ) );
    EXPECT_TRUE( Bitset.Test( 64U ) );
}

TEST( FAtomicBitset, ForEachSetBitAndClear )
{
    LumenEngine::FAtomicBitset Bitset( 128U );

    Bitset.Set( 10U );
    Bitset.Set( 70U );
    Bitset.Set( 100U );

    LumenEngine::TVector<LumenEngine::USize> DetectedBits;
    Bitset.ForEachSetBitAndClear( [&DetectedBits] ( LumenEngine::USize Index ) { DetectedBits.push_back( Index ); } );

    ASSERT_EQ( DetectedBits.size(), 3U );
    EXPECT_EQ( DetectedBits[0], 10U );
    EXPECT_EQ( DetectedBits[1], 70U );
    EXPECT_EQ( DetectedBits[2], 100U );

    EXPECT_FALSE( Bitset.Test( 10U ) );
    EXPECT_FALSE( Bitset.Test( 70U ) );
    EXPECT_FALSE( Bitset.Test( 100U ) );
}

TEST( FAtomicBitset, ConcurrentSet )
{
    LumenEngine::FAtomicBitset Bitset( 1000U );
    constexpr LumenEngine::Int32 NumThreads    = 10;
    constexpr LumenEngine::Int32 BitsPerThread = 100;

    LumenEngine::TVector<std::thread> Threads;
    for ( LumenEngine::Int32 Index = 0; Index < NumThreads; ++Index )
    {
        Threads.emplace_back(
            [&Bitset, Index, BitsPerThread] ()
            {
                for ( LumenEngine::Int32 BitsIndex = 0; BitsIndex < BitsPerThread; ++BitsIndex )
                {
                    Bitset.Set( ( static_cast<LumenEngine::USize>( Index ) * static_cast<LumenEngine::USize>( BitsPerThread ) ) +
                                static_cast<LumenEngine::USize>( BitsIndex ) );
                }
            } );
    }

    for ( auto &Thread : Threads )
    {
        Thread.join();
    }

    LumenEngine::Int32 Count = 0;
    Bitset.ForEachSetBitAndClear( [&Count] ( LumenEngine::USize ) { Count++; } );

    EXPECT_EQ( Count, 1000 );
}
