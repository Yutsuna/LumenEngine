/**
 * @file RHIPipelineCompatibilityTests.cpp
 * @brief Tests pipeline compatibility equality and hashing.
 */

#include <Pipelines/RHIPipelineCompatibility.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIPipelineCompatibility, EqualityAndHashCoverEveryField )
    {
        FRHIPipelineCompatibility Reference;
        Reference.LayoutHash           = 11ULL;
        Reference.ColorFormats         = { ERHIFormat::RGBA8Unorm, ERHIFormat::RG16Float };
        Reference.DepthStencilFormat   = ERHIFormat::D24UnormS8Uint;
        Reference.SampleCount          = ERHISampleCount::Count4;
        Reference.VertexLayoutHash     = 17ULL;
        FRHIPipelineCompatibility Same = Reference;
        LUMEN_CHECK_TRUE( Reference == Same );
        LUMEN_CHECK_EQ( Reference.GetHash(), Same.GetHash() );
        Same.ColorFormats[0] = ERHIFormat::BGRA8Unorm;
        LUMEN_CHECK_FALSE( Reference == Same );
        LUMEN_CHECK_NOT_EQ( Reference.GetHash(), Same.GetHash() );
        Same                    = Reference;
        Same.DepthStencilFormat = ERHIFormat::D32Float;
        LUMEN_CHECK_NOT_EQ( Reference.GetHash(), Same.GetHash() );
        Same             = Reference;
        Same.SampleCount = ERHISampleCount::Count1;
        LUMEN_CHECK_NOT_EQ( Reference.GetHash(), Same.GetHash() );
        Same                  = Reference;
        Same.VertexLayoutHash = 18ULL;
        LUMEN_CHECK_NOT_EQ( Reference.GetHash(), Same.GetHash() );
    }

} // namespace
} // namespace Lumen
