/**
 * @file RHIQueueConformanceTests.cpp
 * @brief Runs portable queue signal and completion scenarios.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"
#include <Commands/IRHICommandAllocator.hpp>
#include <Commands/IRHICommandList.hpp>
#include <Device/IRHIDevice.hpp>
#include <Queue/IRHIQueue.hpp>
#include <Synchronization/IRHISemaphore.hpp>
#include <Tests/UnitTest.hpp>
namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIQueueConformance, SubmitsSignalsAndWaitsForCompletion )
    {
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TSharedRef<IRHIQueue> Queue         = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        TSharedRef<IRHISemaphore> Semaphore = *( *Device )->CreateSemaphore( {} );
        FRHISubmitDesc Desc;
        Desc.Signals    = { { .Semaphore = Semaphore, .Value = 7ULL } };
        Desc.Completion = { 1ULL };
        LUMEN_CHECK_TRUE( Queue->Submit( Desc ).HasValue() );
        LUMEN_CHECK_EQ( Semaphore->GetValue(), 7ULL );
        LUMEN_CHECK_TRUE( Queue->WaitIdle().HasValue() );
        LUMEN_CHECK_EQ( Queue->GetCompletedValue().Value, 1ULL );
        LUMEN_CHECK_TRUE( Semaphore->Wait( 7ULL, 0.0 ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( ( *Device )->GetQueue( ERHIQueueType::Copy, 99U ) );
    }

    LUMEN_TEST( RHIQueueConformance, RejectsRegressionUnsignaledWaitsAndInvalidLists )
    {
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TSharedRef<IRHIQueue> Queue             = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        TSharedRef<IRHISemaphore> Semaphore     = *( *Device )->CreateSemaphore( {} );
        TSharedRef<IRHISemaphore> WorkSemaphore = *( *Device )->CreateSemaphore( {} );
        FRHISubmitDesc Desc;
        Desc.Completion                             = { 1ULL };
        Desc.Waits                                  = { { .Semaphore = Semaphore, .Value = 3ULL } };
        Desc.Signals                                = { { .Semaphore = WorkSemaphore, .Value = 1ULL } };
        const TExpected<void, FRHIError> Unsignaled = Queue->Submit( Desc );
        LUMEN_CHECK_HAS_ERROR( Unsignaled );
        LUMEN_CHECK_EQ( Unsignaled.Error().Code, ERHIErrorCode::InvalidState );
        LUMEN_CHECK_EQ( Unsignaled.Error().Condition, ERHIErrorCondition::Queue );
        LUMEN_CHECK_TRUE( Semaphore->Signal( 3ULL ).HasValue() );
        LUMEN_CHECK_TRUE( Queue->Submit( Desc ).HasValue() );
        const TExpected<void, FRHIError> Regressed = Queue->Submit( Desc );
        LUMEN_CHECK_HAS_ERROR( Regressed );
        LUMEN_CHECK_EQ( Regressed.Error().Condition, ERHIErrorCondition::Queue );
        const TExpected<void, FRHIError> Timeout = Queue->Wait( { 99ULL }, 0.0 );
        LUMEN_CHECK_HAS_ERROR( Timeout );
        LUMEN_CHECK_EQ( Timeout.Error().Code, ERHIErrorCode::Timeout );

        TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
        TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
        FRHISubmitDesc InvalidList;
        InvalidList.CommandLists = { List };
        InvalidList.Completion   = { 2ULL };
        LUMEN_CHECK_HAS_ERROR( Queue->Submit( InvalidList ) );

        TSharedRef<IRHICommandAllocator> ComputeAllocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Compute );
        TSharedRef<IRHICommandList> ComputeList           = *( *Device )->CreateCommandList( ComputeAllocator );
        LUMEN_CHECK_TRUE( ComputeList->Begin().HasValue() );
        LUMEN_CHECK_TRUE( ComputeList->End().HasValue() );
        FRHISubmitDesc WrongQueue;
        WrongQueue.CommandLists                   = { ComputeList };
        WrongQueue.Completion                     = { 2ULL };
        const TExpected<void, FRHIError> Mismatch = Queue->Submit( WrongQueue );
        LUMEN_CHECK_HAS_ERROR( Mismatch );
        LUMEN_CHECK_EQ( Mismatch.Error().Code, ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( Mismatch.Error().Condition, ERHIErrorCondition::Queue );
    }

} // namespace
} // namespace Lumen
