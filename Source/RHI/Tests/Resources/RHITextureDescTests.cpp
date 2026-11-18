/**
 * @file RHITextureDescTests.cpp
 * @brief Tests texture descriptor validation.
 */

#include <Resources/RHITextureDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHITextureDesc, ValidatesFormatExtentAndCounts )
{
    const FRHITextureDesc Valid{ .Extent         = { 32U, 16U, 1U },
                                 .Format         = ERHIFormat::RGBA8Unorm,
                                 .Usage          = ERHIResourceUsage::RenderTarget,
                                 .OptimizedClear = {} };

    LUMEN_CHECK_TRUE( Valid.Validate().HasValue() );
    FRHITextureDesc Invalid = Valid;
    Invalid.MipCount        = 0U;
    LUMEN_CHECK_HAS_ERROR( Invalid.Validate() );
    Invalid        = Valid;
    Invalid.Format = ERHIFormat::Unknown;
    LUMEN_CHECK_HAS_ERROR( Invalid.Validate() );
    Invalid = Valid;
    /** NOTE: A 32x16 extent supports at most 6 mip levels. */
    Invalid.MipCount = 64U;
    LUMEN_CHECK_HAS_ERROR( Invalid.Validate() );
}

} // namespace Lumen
