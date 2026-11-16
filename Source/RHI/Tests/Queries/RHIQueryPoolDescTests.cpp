/**
 * @file RHIQueryPoolDescTests.cpp
 * @brief Tests query result layouts and descriptor validation.
 */

#include <Queries/RHIQueryPoolDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIQueryPoolDesc, ComputesPackedResultSizes )
    {
        LUMEN_CHECK_EQ( GetRHIQueryValueCount( ERHIQueryType::Occlusion ), 1U );
        LUMEN_CHECK_EQ( GetRHIQueryResultSize( ERHIQueryType::Timestamp ), 8U );
        LUMEN_CHECK_EQ( GetRHIQueryResultSize( ERHIQueryType::PipelineStatistics ), 88U );
        LUMEN_CHECK_EQ( GetRHIQueryResultSize( ERHIQueryType::Count ), 0U );
        FRHIQueryPoolDesc Desc;
        Desc.Count = 4U;
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        LUMEN_CHECK_EQ( Desc.GetResultSize(), 8U );
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
} // namespace Lumen
