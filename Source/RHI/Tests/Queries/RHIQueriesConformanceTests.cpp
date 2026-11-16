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
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        FRHIQueryPoolDesc Desc;
        Desc.Count = 2U;
        auto Pool  = ( *Device )->CreateQueryPool( Desc );
        LUMEN_CHECK_TRUE( Pool.HasValue() );
        auto Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
        auto List      = *( *Device )->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->BeginQuery( **Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndQuery( **Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->BeginQuery( **Pool, 1U ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndQuery( **Pool, 1U ).HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        auto Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Completion   = { 1ULL };
        LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
        TArray<UInt64, 2U> Results = {};
        LUMEN_CHECK_TRUE( ( *Pool )->ReadResults( 0U, 2U, Results, Queue->GetCompletedValue() ).HasValue() );
        LUMEN_CHECK_EQ( Results[0], 1ULL );
        LUMEN_CHECK_EQ( Results[1], 2ULL );
    }

    LUMEN_TEST( RHIQueriesConformance, CreatesEveryAdvertisedQueryType )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        FRHIQueryPoolDesc Desc;
        Desc.Type      = ERHIQueryType::Timestamp;
        Desc.QueueType = ERHIQueueType::Compute;
        LUMEN_CHECK_TRUE( ( *Device )->CreateQueryPool( Desc ).HasValue() );
        Desc.Type      = ERHIQueryType::PipelineStatistics;
        Desc.QueueType = ERHIQueueType::Graphics;
        LUMEN_CHECK_TRUE( ( *Device )->CreateQueryPool( Desc ).HasValue() );
        Desc.QueueType = ERHIQueueType::Copy;
        LUMEN_CHECK_HAS_ERROR( ( *Device )->CreateQueryPool( Desc ) );
    }

} // namespace
} // namespace Lumen
