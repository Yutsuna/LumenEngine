/**
 * @file NullQueueTests.cpp
 * @brief Tests Null queue ordering, binary waits, and loss.
 */

#include "Queue/NullQueue.hpp"
#include "Synchronization/NullSynchronization.hpp"
#include <Tests/UnitTest.hpp>
namespace Lumen
{
namespace
{
    LUMEN_TEST( NullQueue, SignalsTimelinesConsumesBinaryWaitsAndRejectsRegression )
    {
        auto Timeline = MakeShared<Tests::FNullSemaphore>( FRHISemaphoreDesc{}, FRHIObjectId{ 1ULL } );
        auto Binary   = MakeShared<Tests::FNullSemaphore>(
            FRHISemaphoreDesc{ .Type = ERHISemaphoreType::Binary, .InitialValue = 1ULL, .DebugName = "Binary" },
            FRHIObjectId{ 2ULL } );
        Tests::FNullQueue Queue( ERHIQueueType::Graphics, 0U, nullptr, FRHIObjectId{ 3ULL } );
        FRHISubmitDesc Desc;
        Desc.Waits      = { { .Semaphore = Binary, .Value = 1ULL } };
        Desc.Signals    = { { .Semaphore = Timeline, .Value = 4ULL } };
        Desc.Completion = { 1ULL };
        LUMEN_CHECK_TRUE( Queue.Submit( Desc ).HasValue() );
        LUMEN_CHECK_EQ( Timeline->GetValue(), 4ULL );
        LUMEN_CHECK_EQ( Binary->GetValue(), 0ULL );
        LUMEN_CHECK_EQ( Queue.GetCompletedValue().Value, 1ULL );
        LUMEN_CHECK_EQ( Queue.GetSubmitCount(), 1ULL );
        LUMEN_CHECK_HAS_ERROR( Queue.Submit( Desc ) );
        LUMEN_CHECK_TRUE( Queue.Wait( { 1ULL }, 0.0 ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( Queue.Wait( { 2ULL }, 0.0 ) );
        Queue.MarkLost();
        LUMEN_CHECK_HAS_ERROR( Queue.WaitIdle() );
    }

} // namespace
} // namespace Lumen
