/**
 * @file RHIResourcesConformanceTests.cpp
 * @brief Runs portable resource creation and views.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"
#include <Device/IRHIDevice.hpp>
#include <Resources/IRHITexture.hpp>
#include <Resources/IRHITextureView.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIResourcesConformance, CreatesTextureAndCompatibleView )
{
    auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
    const FRHITextureDesc Desc{ .Extent         = { 16U, 16U, 1U },
                                .Format         = ERHIFormat::RGBA8Unorm,
                                .Usage          = ERHIResourceUsage::ShaderResource,
                                .OptimizedClear = {} };
    auto Texture = ( *Device )->CreateTexture( Desc );
    LUMEN_CHECK_TRUE( Texture.HasValue() );
    FRHITextureViewDesc ViewDesc;
    ViewDesc.Texture = *Texture;
    auto View        = ( *Device )->CreateTextureView( ViewDesc );
    LUMEN_CHECK_TRUE( View.HasValue() );
    LUMEN_CHECK_EQ( ( *View )->GetTexture()->GetId().GetValue(), ( *Texture )->GetId().GetValue() );
}

} // namespace Lumen
