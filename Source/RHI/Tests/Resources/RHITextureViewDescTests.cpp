/**
 * @file RHITextureViewDescTests.cpp
 * @brief Tests texture-view parent and range validation.
 */

#include <Device/NullDevice.hpp>

#include <Resources/IRHITexture.hpp>
#include <Resources/RHITextureViewDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHITextureViewDesc, ValidatesAgainstParent )
{
    Tests::FNullDevice Device;
    FRHITextureDesc TextureDesc;
    TextureDesc.Extent                                          = { 16U, 16U, 1U };
    TextureDesc.Format                                          = ERHIFormat::RGBA8Unorm;
    TextureDesc.MipCount                                        = 4U;
    const TExpected<TSharedRef<IRHITexture>, FRHIError> Texture = Device.CreateTexture( TextureDesc );
    FRHITextureViewDesc Desc;
    Desc.Texture = *Texture;
    LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
    Desc.Range.BaseMip  = 3U;
    Desc.Range.MipCount = 2U;
    LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    Desc.Range  = {};
    Desc.Format = ERHIFormat::R8Unorm;
    LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
}

} // namespace Lumen
