/**
 * @file NullQueueTests.cpp
 * @brief Tests Null queue ordering, binary waits, and loss.
 */

#include "Queue/NullQueue.hpp"
#include "Synchronization/NullSynchronization.hpp"
#include <Commands/NullCommands.hpp>
#include <Tests/UnitTest.hpp>
namespace Lumen
{

namespace RHI
{
    namespace
    {
        LUMEN_TEST( NullQueue, SignalsTimelinesConsumesBinaryWaitsAndRejectsRegression )
        {
            TSharedRef<Tests::FNullSemaphore> Timeline =
                MakeShared<Tests::FNullSemaphore>( FRHISemaphoreDesc{}, FRHIObjectId{ 1ULL } );
            TSharedRef<Tests::FNullSemaphore> Binary = MakeShared<Tests::FNullSemaphore>(
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

        LUMEN_TEST( NullQueue, SignalFailureLeavesCommandListExecutableAndResubmittable )
        {
            TSharedRef<Tests::FNullCommandAllocator> Allocator =
                MakeShared<Tests::FNullCommandAllocator>( ERHIQueueType::Graphics, FRHIObjectId{ 1ULL } );
            TSharedRef<Tests::FNullCommandList> List = MakeShared<Tests::FNullCommandList>( Allocator, FRHIObjectId{ 2ULL } );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            LUMEN_CHECK_TRUE( List->End().HasValue() );
            LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Executable );

            TSharedRef<Tests::FNullSemaphore> Timeline =
                MakeShared<Tests::FNullSemaphore>( FRHISemaphoreDesc{}, FRHIObjectId{ 3ULL } );
            Tests::FNullQueue Queue( ERHIQueueType::Graphics, 0U, nullptr, FRHIObjectId{ 4ULL } );
            FRHISubmitDesc Desc;
            Desc.CommandLists = { StaticCastSharedRef<IRHICommandList>( List ) };
            Desc.Signals      = { { .Semaphore = Timeline, .Value = 4ULL } };
            Desc.Completion   = { 1ULL };

            Timeline->SetFailNextSignal( true );
            const FRHIExpected Failed = Queue.Submit( Desc );
            LUMEN_CHECK_HAS_ERROR( Failed );
            LUMEN_CHECK_EQ( Failed.Error().Code, ERHIErrorCode::InvalidState );
            LUMEN_CHECK_EQ( Failed.Error().Condition, ERHIErrorCondition::Synchronization );
            LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Executable );
            LUMEN_CHECK_EQ( Queue.GetCompletedValue().Value, 0ULL );
            LUMEN_CHECK_EQ( Queue.GetSubmitCount(), 0ULL );
            LUMEN_CHECK_EQ( Timeline->GetValue(), 0ULL );

            Timeline->SetFailNextSignal( false );
            LUMEN_CHECK_TRUE( Queue.Submit( Desc ).HasValue() );
            LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Executable );
            LUMEN_CHECK_EQ( Queue.GetCompletedValue().Value, 1ULL );
            LUMEN_CHECK_EQ( Queue.GetSubmitCount(), 1ULL );
            LUMEN_CHECK_TRUE( Allocator->Reset( { 1ULL } ).HasValue() );
            LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Initial );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
