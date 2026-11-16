/**
 * @file RHIQueriesTierBTests.cpp
 * @brief Tests overlapping queries, lifecycle errors, and statistics layout.
 */

#include <Commands/IRHICommandAllocator.hpp>
#include <Commands/IRHICommandList.hpp>
#include <Device/NullDevice.hpp>
#include <Queries/NullQueries.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIQueriesTierB, RejectsOverlapAndUnfinishedCommandLists )
    {
        Tests::FNullDevice Device;
        FRHIQueryPoolDesc Desc;
        auto Pool      = *Device.CreateQueryPool( Desc );
        auto Allocator = *Device.CreateCommandAllocator( ERHIQueueType::Graphics );
        auto List      = *Device.CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->BeginQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( List->BeginQuery( *Pool, 0U ) );
        LUMEN_CHECK_HAS_ERROR( List->End() );
        LUMEN_CHECK_TRUE( List->EndQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
    }

    LUMEN_TEST( RHIQueriesTierB, ReadsAllPipelineStatisticCounters )
    {
        FRHIQueryPoolDesc Desc;
        Desc.Type = ERHIQueryType::PipelineStatistics;
        Tests::FNullQueryPool Pool( Desc, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 3ULL } );
        Pool.SetNextValue( 100ULL );
        LUMEN_CHECK_TRUE( Pool.Begin( 0U ).HasValue() );
        LUMEN_CHECK_TRUE( Pool.End( 0U ).HasValue() );
        TArray<UInt64, 11U> Results = {};
        LUMEN_CHECK_TRUE( Pool.ReadResults( 0U, 1U, Results, {} ).HasValue() );
        LUMEN_CHECK_EQ( Results[0], 100ULL );
        LUMEN_CHECK_EQ( Results[10], 110ULL );
        LUMEN_CHECK_TRUE( Pool.SetQueryValue( 0U, 500ULL, { 4ULL } ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( Pool.ReadResults( 0U, 1U, Results, { 3ULL } ) );
        LUMEN_CHECK_TRUE( Pool.ReadResults( 0U, 1U, Results, { 4ULL } ).HasValue() );
        LUMEN_CHECK_EQ( Results[0], 500ULL );
    }

} // namespace
} // namespace Lumen
