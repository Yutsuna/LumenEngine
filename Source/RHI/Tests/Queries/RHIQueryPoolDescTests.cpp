/**
 * @file RHIQueryPoolDescTests.cpp
 * @brief Tests query result layouts and descriptor validation.
 */

#include <RHI/Queries/RHIQueryPoolDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHIQueryPoolDesc, ComputesPackedResultSizes )
        {
            LUMEN_CHECK_EQ( GetRHIQueryValueCount( ERHIQueryType::Occlusion ).ValueOr( 0U ), 1U );
            LUMEN_CHECK_EQ( GetRHIQueryValueCount( ERHIQueryType::Timestamp ).ValueOr( 0U ), 1U );
            LUMEN_CHECK_EQ( GetRHIQueryValueCount( ERHIQueryType::PipelineStatistics ).ValueOr( 0U ), 11U );
            LUMEN_CHECK_EQ( GetRHIQueryResultSize( ERHIQueryType::Occlusion ).ValueOr( 0U ), 8U );
            LUMEN_CHECK_EQ( GetRHIQueryResultSize( ERHIQueryType::Timestamp ).ValueOr( 0U ), 8U );
            LUMEN_CHECK_EQ( GetRHIQueryResultSize( ERHIQueryType::PipelineStatistics ).ValueOr( 0U ), 88U );
            FRHIQueryPoolDesc Desc;
            Desc.Count = 4U;
            LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
            LUMEN_CHECK_EQ( Desc.GetResultSize(), 8U );
        }

        LUMEN_TEST( RHIQueryPoolDesc, QueryLayoutHelpersRejectInvalidType )
        {
            LUMEN_CHECK_FALSE( GetRHIQueryValueCount( ERHIQueryType::Count ).HasValue() );
            LUMEN_CHECK_FALSE( GetRHIQueryResultSize( ERHIQueryType::Count ).HasValue() );
        }

        LUMEN_TEST( RHIQueryPoolDesc, RejectsInvalidTypesCountsAndQueues )
        {
            FRHIQueryPoolDesc Desc;
            Desc.Count = 0U;
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc.Count     = 1U;
            Desc.QueueType = ERHIQueueType::Copy;
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc.Type      = ERHIQueryType::Timestamp;
            Desc.QueueType = ERHIQueueType::Compute;
            LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
            Desc.Type = ERHIQueryType::PipelineStatistics;
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
