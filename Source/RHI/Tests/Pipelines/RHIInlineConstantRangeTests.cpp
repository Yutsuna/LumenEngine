/**
 * @file RHIInlineConstantRangeTests.cpp
 * @brief Tests inline constant range validation.
 */

#include <Pipelines/RHIInlineConstantRange.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIInlineConstantRange, RequiresAlignedSizeOffsetAndVisibleStages )
    {
        FRHIInlineConstantRange Range{ .Offset = 4U, .Size = 16U, .Stages = ERHIShaderStage::Vertex };
        LUMEN_CHECK_TRUE( Range.Validate().HasValue() );
        Range.Size = 0U;
        LUMEN_CHECK_HAS_ERROR( Range.Validate() );
        Range.Size = 3U;
        LUMEN_CHECK_HAS_ERROR( Range.Validate() );
        Range.Size   = 16U;
        Range.Offset = 2U;
        LUMEN_CHECK_HAS_ERROR( Range.Validate() );
        Range.Offset = 4U;
        Range.Stages = ERHIShaderStage::None;
        LUMEN_CHECK_HAS_ERROR( Range.Validate() );
    }

} // namespace
} // namespace Lumen
