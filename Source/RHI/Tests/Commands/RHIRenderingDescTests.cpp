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
        TSharedRef<IRHITexture> Texture = *Device.CreateTexture( { .Extent         = { 16U, 8U, 1U },
                                                                   .Format         = ERHIFormat::RGBA8Unorm,
                                                                   .Usage          = ERHIResourceUsage::RenderTarget,
                                                                   .OptimizedClear = {} } );
        FRHITextureViewDesc ViewDesc;
        ViewDesc.Texture                 = Texture;
        TSharedRef<IRHITextureView> View = *Device.CreateTextureView( ViewDesc );
        FRHIRenderingDesc Desc;
        Desc.Area = { 0, 0, 16U, 8U };
        Desc.ColorAttachments.push_back(
            { .View = View, .LoadOp = ERHIRenderingLoadOp::Clear, .StoreOp = ERHIRenderingStoreOp::Store, .ClearValue = {} } );
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        Desc.Area.Size.Width = 17U;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Area.Size.Width                = 16U;
        Desc.ColorAttachments[0].ClearValue = FRHIDepthStencilClearValue{};
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.ColorAttachments[0].ClearValue = FRHIColorClearValue{};
        Desc.ColorAttachments[0].LoadOp     = static_cast<ERHIRenderingLoadOp>( 3U );
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }
    LUMEN_TEST( RHIRenderingDesc, RequiresAtLeastOneAttachment )
    {
        FRHIRenderingDesc Desc;
        Desc.Area = { 0, 0, 1U, 1U };
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }

} // namespace

} // namespace Lumen
