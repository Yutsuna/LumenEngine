/**
 * @file RHIQueriesConformanceTests.cpp
 * @brief Runs portable query recording and readback scenarios.
 */

#include <Commands/IRHICommandAllocator.hpp>
#include <Commands/IRHICommandList.hpp>
#include <Device/IRHIDevice.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>
#include <Queries/IRHIQueryPool.hpp>
#include <Queue/IRHIQueue.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIQueriesConformance, RecordsOcclusionQueriesAndReadsResults )
    {
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        if ( not( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::OcclusionQueries ) )
        {
            return;
        }
        FRHIQueryPoolDesc Desc;
        Desc.Count                                           = 2U;
        TExpected<TSharedRef<IRHIQueryPool>, FRHIError> Pool = ( *Device )->CreateQueryPool( Desc );
        LUMEN_CHECK_TRUE( Pool.HasValue() );
        TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
        TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->BeginQuery( **Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndQuery( **Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->BeginQuery( **Pool, 1U ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndQuery( **Pool, 1U ).HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Completion   = { 1ULL };
        LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
        TArray<UInt64, 2U> Results = {};
        LUMEN_CHECK_TRUE( ( *Pool )->ReadResults( 0U, 2U, Results, Queue->GetCompletedValue() ).HasValue() );
        LUMEN_CHECK_EQ( Results[0], 1ULL );
        LUMEN_CHECK_EQ( Results[1], 2ULL );
    }

    LUMEN_TEST( RHIQueriesConformance, QueryTypesFollowAdvertisedCapabilities )
    {
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const TArray<ERHIQueryType, 3U> Types          = { ERHIQueryType::Occlusion, ERHIQueryType::Timestamp,
                                                           ERHIQueryType::PipelineStatistics };
        const TArray<ERHICapabilityFlags, 3U> Required = { ERHICapabilityFlags::OcclusionQueries,
                                                           ERHICapabilityFlags::TimestampQueries,
                                                           ERHICapabilityFlags::PipelineStatistics };
        for ( USize Index = 0U; Index < Types.size(); ++Index )
        {
            FRHIQueryPoolDesc Desc;
            Desc.Type      = Types[Index];
            Desc.QueueType = Desc.Type == ERHIQueryType::Timestamp ? ERHIQueueType::Compute : ERHIQueueType::Graphics;
            TExpected<TSharedRef<IRHIQueryPool>, FRHIError> Pool = ( *Device )->CreateQueryPool( Desc );
            if ( ( *Device )->GetCapabilities().Supports( Required[Index] ) )
            {
                LUMEN_CHECK_TRUE( Pool.HasValue() );
            }
            else
            {
                LUMEN_CHECK_HAS_ERROR( Pool );
                LUMEN_CHECK_EQ( Pool.Error().Code, ERHIErrorCode::Unsupported );
                LUMEN_CHECK_EQ( Pool.Error().Condition, ERHIErrorCondition::Query );
            }
        }

        FRHIQueryPoolDesc InvalidQueueDesc;
        InvalidQueueDesc.Type                                              = ERHIQueryType::PipelineStatistics;
        InvalidQueueDesc.QueueType                                         = ERHIQueueType::Copy;
        const TExpected<TSharedRef<IRHIQueryPool>, FRHIError> InvalidQueue = ( *Device )->CreateQueryPool( InvalidQueueDesc );
        LUMEN_CHECK_HAS_ERROR( InvalidQueue );
        LUMEN_CHECK_EQ( InvalidQueue.Error().Code, ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( InvalidQueue.Error().Condition, ERHIErrorCondition::Query );
    }

    LUMEN_TEST( RHIQueriesConformance, ResetMakesResultsUnavailableAndChecksRanges )
    {
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        if ( not( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::OcclusionQueries ) )
        {
            return;
        }
        TSharedRef<IRHIQueryPool> Pool             = *( *Device )->CreateQueryPool( {} );
        TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
        TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        const TExpected<void, FRHIError> InvalidIndex = List->BeginQuery( *Pool, 1U );
        LUMEN_CHECK_HAS_ERROR( InvalidIndex );
        LUMEN_CHECK_EQ( InvalidIndex.Error().Condition, ERHIErrorCondition::Query );
        LUMEN_CHECK_TRUE( List->BeginQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Completion   = { 1ULL };
        LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );

        TArray<UInt64, 1U> Result = {};
        LUMEN_CHECK_TRUE( Pool->ReadResults( 0U, 1U, Result, Queue->GetCompletedValue() ).HasValue() );
        Pool->Reset();
        const TExpected<void, FRHIError> Unavailable = Pool->ReadResults( 0U, 1U, Result, Queue->GetCompletedValue() );
        LUMEN_CHECK_HAS_ERROR( Unavailable );
        LUMEN_CHECK_EQ( Unavailable.Error().Code, ERHIErrorCode::InvalidState );
        const TExpected<void, FRHIError> InvalidRange = Pool->ReadResults( 0U, 2U, Result, Queue->GetCompletedValue() );
        LUMEN_CHECK_HAS_ERROR( InvalidRange );
        LUMEN_CHECK_EQ( InvalidRange.Error().Code, ERHIErrorCode::InvalidArgument );
    }

} // namespace
} // namespace Lumen
