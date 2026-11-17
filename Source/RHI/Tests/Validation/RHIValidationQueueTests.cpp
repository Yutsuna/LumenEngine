/**
 * @file RHIValidationQueueTests.cpp
 * @brief Tests executable-state, queue-type, semaphore, and timeline validation.
 */
#include "Validation/RHIValidationReporter.hpp"
#include "Validation/RHIValidationTestUtils.hpp"
#include <Queue/IRHIQueue.hpp>
#include <Synchronization/IRHISemaphore.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIValidationQueue, RejectsNonExecutableListsAndMonotonicityViolations )
    {
        Tests::FRecordingDebugSink Sink;
        TSharedRef<IRHIDevice> Device              = Tests::Validation::MakeDevice( Sink );
        TSharedRef<IRHIQueue> Queue                = *Device->GetQueue( ERHIQueueType::Graphics, 0U );
        TSharedRef<IRHICommandAllocator> Allocator = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
        TSharedRef<IRHICommandList> List           = *Device->CreateCommandList( Allocator );
        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Completion   = { 1ULL };
        LUMEN_CHECK_EQ( Queue->Submit( Submit ).Error().Code, ERHIErrorCode::ValidationFailed );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
        LUMEN_CHECK_EQ( Queue->Submit( Submit ).Error().Code, ERHIErrorCode::ValidationFailed );
        TSharedRef<IRHIQueue> ComputeQueue = *Device->GetQueue( ERHIQueueType::Compute, 0U );
        Submit.Completion                  = { 2ULL };
        LUMEN_CHECK_EQ( ComputeQueue->Submit( Submit ).Error().Code, ERHIErrorCode::ValidationFailed );
        LUMEN_CHECK_TRUE( Sink.ContainsCode( static_cast<UInt32>( ERHIValidationMessageCode::InvalidQueueSubmission ) ) );
    }

    LUMEN_TEST( RHIValidationQueue, RejectsUnsignaledWaitsBeforeBackendSubmission )
    {
        Tests::FRecordingDebugSink Sink;
        TSharedRef<IRHIDevice> Device              = Tests::Validation::MakeDevice( Sink );
        TSharedRef<IRHIQueue> Queue                = *Device->GetQueue( ERHIQueueType::Graphics, 0U );
        TSharedRef<IRHICommandAllocator> Allocator = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
        TSharedRef<IRHICommandList> List           = *Device->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        FRHISemaphoreDesc SemaphoreDesc;
        SemaphoreDesc.Type                  = ERHISemaphoreType::Timeline;
        TSharedRef<IRHISemaphore> Semaphore = *Device->CreateSemaphore( SemaphoreDesc );
        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Waits.push_back( { .Semaphore = Semaphore, .Value = 1ULL } );
        Submit.Completion = { 1ULL };
        LUMEN_CHECK_EQ( Queue->Submit( Submit ).Error().Code, ERHIErrorCode::ValidationFailed );
        LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Executable );
        LUMEN_CHECK_EQ( Queue->GetCompletedValue().Value, 0ULL );
    }
} // namespace
} // namespace Lumen
