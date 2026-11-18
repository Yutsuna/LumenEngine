/**
 * @file RHIPipelineLayoutDescTests.cpp
 * @brief Tests inline constant layout overlap validation.
 */

#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <Tests/UnitTest.hpp>
#include <limits>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHIPipelineLayoutDesc, AcceptsDisjointRangesAndRejectsOverlap )
        {
            FRHIPipelineLayoutDesc Desc;
            Desc.InlineConstants.push_back( { .Offset = 0U, .Size = 16U, .Stages = ERHIShaderStage::Vertex } );
            Desc.InlineConstants.push_back( { .Offset = 16U, .Size = 8U, .Stages = ERHIShaderStage::Pixel } );
            LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
            Desc.InlineConstants[1].Offset = 12U;
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        }

        LUMEN_TEST( RHIPipelineLayoutDesc, RejectsInvalidRangesAndOverflow )
        {
            FRHIPipelineLayoutDesc Desc;
            Desc.InlineConstants.push_back( {} );
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc.InlineConstants[0] = {
                .Offset = std::numeric_limits<UInt32>::max() - 3U, .Size = 4U, .Stages = ERHIShaderStage::Compute };
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
