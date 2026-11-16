/**
 * @file RHIQueueStressTests.cpp
 * @brief Stresses queue completion with concurrent waiters.
 */

#include "Queue/NullQueue.hpp"
#include "Synchronization/NullSynchronization.hpp"
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>
namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIQueueStress, ConcurrentWaitersObserveEveryCompletedValue )
    {
        auto Semaphore = MakeShared<Tests::FNullSemaphore>( FRHISemaphoreDesc{}, FRHIObjectId{ 1ULL } );
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

} // namespace
} // namespace Lumen
