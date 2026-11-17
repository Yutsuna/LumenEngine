/**
 * @file RHIQueueStressTests.cpp
 * @brief Stresses queue completion with concurrent waiters.
 */

#include "Queue/NullQueue.hpp"
#include "Synchronization/NullSynchronization.hpp"
#include <Commands/NullCommands.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>
namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIQueueStress, ConcurrentWaitersObserveEveryCompletedValue )
    {
        TSharedRef<Tests::FNullSemaphore> Semaphore =
            MakeShared<Tests::FNullSemaphore>( FRHISemaphoreDesc{}, FRHIObjectId{ 1ULL } );
        Tests::FNullQueue Queue( ERHIQueueType::Graphics, 0U, nullptr, FRHIObjectId{ 2ULL } );
        TAtomic<UInt32> Failures{ 0U };
        TVector<FThread> Waiters;
        for ( UInt32 Index = 0U; Index < 4U; ++Index )
        {
            Waiters.emplace_back( [&Queue, &Failures]
            {
                if ( not Queue.Wait( { 128ULL }, 5.0 ) )
                {
                    Failures.fetch_add( 1U );
                }
            } );
        }
        for ( UInt64 Value = 1ULL; Value <= 128ULL; ++Value )
        {
            FRHISubmitDesc Desc;
            Desc.Signals    = { { .Semaphore = Semaphore, .Value = Value } };
            Desc.Completion = { Value };
            if ( not Queue.Submit( Desc ) )
            {
                Failures.fetch_add( 1U );
            }
        }
        for ( auto &Waiter : Waiters )
        {
            Waiter.Join();
        }
        LUMEN_CHECK_EQ( Failures.load(), 0U );
        LUMEN_CHECK_EQ( Queue.GetSubmitCount(), 128ULL );
    }

    LUMEN_TEST( RHIQueueStress, ConcurrentProducersSerializeSubmissionsAndSignals )
    {
        Tests::FNullQueue Queue( ERHIQueueType::Graphics, 0U, nullptr, FRHIObjectId{ 2ULL } );
        TAtomic<UInt64> NextCompletion{ 1ULL };
        TAtomic<UInt32> Successes{ 0U };
        TAtomic<UInt32> Failures{ 0U };
        TVector<FThread> Producers;
        for ( UInt32 Producer = 0U; Producer < 4U; ++Producer )
        {
            Producers.emplace_back( [&Queue, &NextCompletion, &Successes, &Failures, Producer]
            {
                TSharedRef<Tests::FNullCommandAllocator> Allocator =
                    MakeShared<Tests::FNullCommandAllocator>( ERHIQueueType::Graphics, FRHIObjectId{ 100ULL + Producer * 2ULL } );
                TSharedRef<Tests::FNullCommandList> List =
                    MakeShared<Tests::FNullCommandList>( Allocator, FRHIObjectId{ 101ULL + Producer * 2ULL } );
                if ( not List->Begin() or not List->End() )
                {
                    Failures.fetch_add( 1U );
                    return;
                }
                TSharedRef<Tests::FNullSemaphore> Semaphore =
                    MakeShared<Tests::FNullSemaphore>( FRHISemaphoreDesc{}, FRHIObjectId{ 200ULL + Producer } );
                UInt32 Completed = 0U;
                UInt32 Attempts  = 0U;
                while ( Completed < 64U and Attempts < 4096U )
                {
                    ++Attempts;
                    FRHISubmitDesc Desc;
                    Desc.CommandLists = { StaticCastSharedRef<IRHICommandList>( List ) };
                    Desc.Signals      = { { .Semaphore = Semaphore, .Value = Completed + 1ULL } };
                    Desc.Completion   = { NextCompletion.fetch_add( 1ULL ) };
                    if ( Queue.Submit( Desc ) )
                    {
                        ++Completed;
                        Successes.fetch_add( 1U );
                    }
                }
                if ( Completed != 64U or Semaphore->GetValue() != 64ULL )
                {
                    Failures.fetch_add( 1U );
                }
            } );
        }
        for ( auto &Producer : Producers )
        {
            Producer.Join();
        }
        LUMEN_CHECK_EQ( Failures.load(), 0U );
        LUMEN_CHECK_EQ( Successes.load(), 256U );
        LUMEN_CHECK_EQ( Queue.GetSubmitCount(), 256ULL );
        LUMEN_CHECK_TRUE( Queue.GetCompletedValue().IsValid() );
        LUMEN_CHECK_TRUE( Queue.WaitIdle().HasValue() );
    }

} // namespace
} // namespace Lumen
