/**
 * @file SPMCQueueTest.cpp
 * @brief Unit tests for the TSPMCQueue class.
 */

#include "Thread/SPMCQueue.hpp"
#include "Container/Vector.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>

TEST( ParallelSPMCQueue, BasicPushPop )
{
    LumenEngine::Parallel::TSPMCQueue<LumenEngine::Int32> Queue( 4ULL );

    EXPECT_TRUE( Queue.Push( 10 ) );
    EXPECT_TRUE( Queue.Push( 20 ) );

    LumenEngine::TOptional<LumenEngine::Int32> Val1 = Queue.Pop();
    ASSERT_TRUE( Val1.has_value() );
    EXPECT_EQ( *Val1, 10 );

    LumenEngine::TOptional<LumenEngine::Int32> Val2 = Queue.Pop();
    ASSERT_TRUE( Val2.has_value() );
    EXPECT_EQ( *Val2, 20 );

    EXPECT_FALSE( Queue.Pop().has_value() );
}

TEST( ParallelSPMCQueue, CapacityLimit )
{
    LumenEngine::Parallel::TSPMCQueue<LumenEngine::Int32> Queue( 2ULL );

    EXPECT_TRUE( Queue.Push( 1 ) );
    EXPECT_TRUE( Queue.Push( 2 ) );
    EXPECT_FALSE( Queue.Push( 3 ) ); /** < Queue is full */

    EXPECT_EQ( *Queue.Pop(), 1 );
    EXPECT_TRUE( Queue.Push( 3 ) ); /** < Slot freed, now it succeeds */
}

TEST( ParallelSPMCQueue, MultiConsumerSafety )
{
    LumenEngine::Parallel::TSPMCQueue<LumenEngine::Int32> Queue( 1024ULL );

    constexpr LumenEngine::Int32 TotalMessages = 50000;

    LumenEngine::TAtomic<LumenEngine::Int32> ConsumedCount = 0;
    LumenEngine::TVector<std::jthread> Consumers;

    for ( LumenEngine::Int32 Index = 0; Index < 4; ++Index )
    {
        Consumers.emplace_back(
            [&] ()
            {
                while ( ConsumedCount.load( std::memory_order_acquire ) < TotalMessages )
                {
                    if ( LumenEngine::TOptional<LumenEngine::Int32> Val = Queue.Pop() )
                    {
                        ConsumedCount.fetch_add( 1, std::memory_order_relaxed );
                    }
                }
            } );
    }

    for ( LumenEngine::Int32 Index = 0; Index < TotalMessages; ++Index )
    {
        while ( not Queue.Push( Index ) )
        {
            std::this_thread::yield();
        }
    }

    Consumers.clear(); /** < Joins threads safely */
    EXPECT_EQ( ConsumedCount.load(), TotalMessages );
}