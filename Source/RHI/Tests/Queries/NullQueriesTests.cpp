/**
 * @file NullQueriesTests.cpp
 * @brief Tests deterministic Null query values, reset, and availability.
 */

#include <Queries/NullQueries.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( NullQueries, TracksActiveQueriesAndWritesDeterministicResults )
        {
            FRHIQueryPoolDesc Desc;
            Desc.Count = 2U;
            Tests::FNullQueryPool Pool( Desc, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 1ULL } );
            LUMEN_CHECK_TRUE( Pool.Begin( 0U ).HasValue() );
            LUMEN_CHECK_TRUE( Pool.IsActive( 0U ) );
            LUMEN_CHECK_HAS_ERROR( Pool.Begin( 0U ) );
            Pool.SetNextValue( 42ULL );
            LUMEN_CHECK_TRUE( Pool.End( 0U ).HasValue() );
            LUMEN_CHECK_HAS_ERROR( Pool.End( 0U ) );
            TArray<UInt64, 1U> Result = {};
            LUMEN_CHECK_TRUE( Pool.ReadResults( 0U, 1U, Result, {} ).HasValue() );
            LUMEN_CHECK_EQ( Result[0], 42ULL );
            Pool.Reset();
            LUMEN_CHECK_HAS_ERROR( Pool.ReadResults( 0U, 1U, Result, {} ) );
            LUMEN_CHECK_HAS_ERROR( Pool.Begin( 9U ) );
        }

        LUMEN_TEST( NullQueries, HonorsAvailabilityAndFaultInjection )
        {
            FRHIQueryPoolDesc Desc;
            Desc.Type                                   = ERHIQueryType::Timestamp;
            TSharedRef<Tests::FNullFaultPlan> FaultPlan = MakeShared<Tests::FNullFaultPlan>();
            Tests::FNullQueryPool Pool( Desc, FaultPlan, FRHIObjectId{ 2ULL } );
            Pool.SetNextAvailability( { 10ULL } );
            LUMEN_CHECK_TRUE( Pool.Begin( 0U ).HasValue() );
            LUMEN_CHECK_TRUE( Pool.End( 0U ).HasValue() );
            TArray<UInt64, 1U> Result = {};
            LUMEN_CHECK_HAS_ERROR( Pool.ReadResults( 0U, 1U, Result, { 9ULL } ) );
            LUMEN_CHECK_TRUE( Pool.ReadResults( 0U, 1U, Result, { 10ULL } ).HasValue() );
            FaultPlan->AddFailure( "ReadResults", 1U, MakeRHIBackendFailure( ERHIErrorCondition::Query, "planned" ) );
            LUMEN_CHECK_HAS_ERROR( Pool.ReadResults( 0U, 1U, Result, { 10ULL } ) );
            LUMEN_CHECK_HAS_ERROR( Pool.ReadResults( 0U, 2U, Result, { 10ULL } ) );
        }

        LUMEN_TEST( NullQueries, WritesDeterministicDefaultAndStatisticValues )
        {
            FRHIQueryPoolDesc Desc;
            Desc.Count = 2U;
            Tests::FNullQueryPool Pool( Desc, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 3ULL } );
            LUMEN_CHECK_TRUE( Pool.Begin( 0U ).HasValue() );
            LUMEN_CHECK_TRUE( Pool.End( 0U ).HasValue() );
            LUMEN_CHECK_TRUE( Pool.Begin( 1U ).HasValue() );
            LUMEN_CHECK_TRUE( Pool.End( 1U ).HasValue() );
            TArray<UInt64, 2U> Results = {};
            LUMEN_CHECK_TRUE( Pool.ReadResults( 0U, 2U, Results, {} ).HasValue() );
            LUMEN_CHECK_EQ( Results[0], 1ULL );
            LUMEN_CHECK_EQ( Results[1], 2ULL );

            FRHIQueryPoolDesc StatisticsDesc;
            StatisticsDesc.Type = ERHIQueryType::PipelineStatistics;
            Tests::FNullQueryPool Statistics( StatisticsDesc, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 4ULL } );
            Statistics.SetNextValue( 100ULL );
            LUMEN_CHECK_TRUE( Statistics.Begin( 0U ).HasValue() );
            LUMEN_CHECK_TRUE( Statistics.End( 0U ).HasValue() );
            TArray<UInt64, 11U> StatisticsResults = {};
            LUMEN_CHECK_TRUE( Statistics.ReadResults( 0U, 1U, StatisticsResults, {} ).HasValue() );
            LUMEN_CHECK_EQ( StatisticsResults[0], 100ULL );
            LUMEN_CHECK_EQ( StatisticsResults[10], 110ULL );
            LUMEN_CHECK_TRUE( Statistics.SetQueryValue( 0U, 500ULL, { 4ULL } ).HasValue() );
            LUMEN_CHECK_HAS_ERROR( Statistics.ReadResults( 0U, 1U, StatisticsResults, { 3ULL } ) );
            LUMEN_CHECK_TRUE( Statistics.ReadResults( 0U, 1U, StatisticsResults, { 4ULL } ).HasValue() );
            LUMEN_CHECK_EQ( StatisticsResults[0], 500ULL );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
