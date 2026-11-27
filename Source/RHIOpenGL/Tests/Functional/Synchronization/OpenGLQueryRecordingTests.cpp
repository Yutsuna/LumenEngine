/**
 * @file OpenGLQueryRecordingTests.cpp
 * @brief Functional coverage for executor-published query results in M4.2.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"

#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Queries/IRHIQueryPool.hpp>
#include <RHI/Queue/IRHIQueue.hpp>

#include <Templates/Array.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    using namespace RHI;

    LUMEN_TEST( OpenGLQueryRecording, OcclusionResultsArePublishedAfterReplayAndGatedByCompletion )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );

        FRHIQueryPoolDesc Desc;
        Desc.Count                                                 = 2U;
        const TExpected<TSharedRef<IRHIQueryPool>, FRHIError> Pool = ( *Device )->CreateQueryPool( Desc );
        LUMEN_CHECK_TRUE( Pool.HasValue() );

        TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
        TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->BeginQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->BeginQuery( *Pool, 1U ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndQuery( *Pool, 1U ).HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );

        TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Completion   = { 1ULL };

        TArray<UInt64, 2U> Results = {};
        LUMEN_CHECK_HAS_ERROR( ( *Pool )->ReadResults( 0U, 2U, Results, Queue->GetCompletedValue() ) );
        LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( ( *Pool )->ReadResults( 0U, 2U, Results, Queue->GetCompletedValue() ) );
        LUMEN_CHECK_TRUE( Queue->WaitIdle().HasValue() );
        LUMEN_CHECK_TRUE( ( *Pool )->ReadResults( 0U, 2U, Results, Queue->GetCompletedValue() ).HasValue() );

        ( *Pool )->Reset();
        LUMEN_CHECK_HAS_ERROR( ( *Pool )->ReadResults( 0U, 2U, Results, Queue->GetCompletedValue() ) );
    }

    LUMEN_TEST( OpenGLQueryRecording, TimestampResultsAreMonotonicAndCompletionGated )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        if ( not( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::TimestampQueries ) )
        {
            return;
        }

        FRHIQueryPoolDesc Desc;
        Desc.Type                                                  = ERHIQueryType::Timestamp;
        Desc.QueueType                                             = ERHIQueueType::Compute;
        Desc.Count                                                 = 2U;
        const TExpected<TSharedRef<IRHIQueryPool>, FRHIError> Pool = ( *Device )->CreateQueryPool( Desc );
        LUMEN_CHECK_TRUE( Pool.HasValue() );

        TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Compute );
        TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->BeginQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->BeginQuery( *Pool, 1U ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndQuery( *Pool, 1U ).HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );

        TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Compute, 0U );
        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Completion   = { 1ULL };

        TArray<UInt64, 2U> Results = {};
        LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
        LUMEN_CHECK_TRUE( Queue->WaitIdle().HasValue() );
        LUMEN_CHECK_TRUE( ( *Pool )->ReadResults( 0U, 2U, Results, Queue->GetCompletedValue() ).HasValue() );
        LUMEN_CHECK_TRUE( Results[0] > 0ULL );
        LUMEN_CHECK_TRUE( Results[1] >= Results[0] );
    }

    LUMEN_TEST( OpenGLQueryRecording, RecordingRejectsForeignPoolsOverlapAndOpenQueries )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );

        FRHIQueryPoolDesc Desc;
        const TExpected<TSharedRef<IRHIQueryPool>, FRHIError> Pool = ( *Device )->CreateQueryPool( Desc );
        LUMEN_CHECK_TRUE( Pool.HasValue() );

        TSharedRef<IRHICommandAllocator> GraphicsAllocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
        TSharedRef<IRHICommandList> GraphicsList           = *( *Device )->CreateCommandList( GraphicsAllocator );
        LUMEN_CHECK_TRUE( GraphicsList->Begin().HasValue() );

        const FRHIExpected InvalidIndex = GraphicsList->BeginQuery( *Pool, 1U );
        LUMEN_CHECK_HAS_ERROR( InvalidIndex );
        LUMEN_CHECK_EQ( InvalidIndex.Error().Condition, ERHIErrorCondition::Query );
        LUMEN_CHECK_TRUE( GraphicsList->BeginQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( GraphicsList->BeginQuery( *Pool, 0U ) );
        LUMEN_CHECK_HAS_ERROR( GraphicsList->End() );
        LUMEN_CHECK_TRUE( GraphicsList->EndQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( GraphicsList->End().HasValue() );

        TSharedRef<IRHICommandAllocator> ComputeAllocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Compute );
        TSharedRef<IRHICommandList> ComputeList           = *( *Device )->CreateCommandList( ComputeAllocator );
        LUMEN_CHECK_TRUE( ComputeList->Begin().HasValue() );
        LUMEN_CHECK_HAS_ERROR( ComputeList->BeginQuery( *Pool, 0U ) );
    }

} // namespace

} // namespace Lumen
