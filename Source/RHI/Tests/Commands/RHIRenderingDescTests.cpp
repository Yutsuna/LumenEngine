/**
 * @file RHIRenderingDescTests.cpp
 * @brief Tests dynamic-rendering attachments and render area.
 */

#include "Device/NullDevice.hpp"
#include <Commands/RHIRenderingDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{
    LUMEN_TEST( RHIRenderingDesc, ValidatesAttachmentsClearTypesAndArea )
    {
        Tests::FNullDevice Device;
        auto Texture = *Device.CreateTexture( { .Extent         = { .Width = 16U, .Height = 8U, .Depth = 1U },
                                                .Format         = ERHIFormat::RGBA8Unorm,
                                                .Usage          = ERHIResourceUsage::RenderTarget,
                                                .OptimizedClear = {} } );
        FRHITextureViewDesc ViewDesc;
        ViewDesc.Texture = Texture;
        auto View        = *Device.CreateTextureView( ViewDesc );
        FRHIRenderingDesc Desc;
        Desc.Area = { .X = 0, .Y = 0, .Width = 16U, .Height = 8U };
        Desc.ColorAttachments.push_back(
            { .View = View, .LoadOp = ERHIRenderingLoadOp::Clear, .StoreOp = ERHIRenderingStoreOp::Store, .ClearValue = {} } );
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        Desc.Area.Width = 17U;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Area.Width                     = 16U;
        Desc.ColorAttachments[0].ClearValue = FRHIDepthStencilClearValue{};
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.ColorAttachments[0].ClearValue = FRHIColorClearValue{};
        Desc.ColorAttachments[0].LoadOp     = static_cast<ERHIRenderingLoadOp>( 3U );
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }
    LUMEN_TEST( RHIRenderingDesc, RequiresAtLeastOneAttachment )
    {
        FRHIRenderingDesc Desc;
        Desc.Area = { .X = 0, .Y = 0, .Width = 1U, .Height = 1U };
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }

} // namespace

} // namespace Lumen
