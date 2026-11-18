/**
 * @file RHIRenderingDescTests.cpp
 * @brief Tests dynamic-rendering attachments and render area.
 */

#include <Device/NullDevice.hpp>
#include <RHI/Commands/RHIRenderingDesc.hpp>
#include <Resources/NullResources.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
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
            Desc.ColorAttachments.push_back( { .View       = View,
                                               .LoadOp     = ERHIRenderingLoadOp::Clear,
                                               .StoreOp    = ERHIRenderingStoreOp::Store,
                                               .ClearValue = {} } );
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

        LUMEN_TEST( RHIRenderingDesc, RejectsMipOutsideTextureChain )
        {
            Tests::FNullDevice Device;
            TSharedRef<IRHITexture> Texture = *Device.CreateTexture( { .Extent         = { 100U, 50U, 1U },
                                                                       .Format         = ERHIFormat::RGBA8Unorm,
                                                                       .MipCount       = 7U,
                                                                       .Usage          = ERHIResourceUsage::RenderTarget,
                                                                       .OptimizedClear = {} } );

            /** NOTE: Built directly to bypass view validation, reproducing the untrusted shape that used to shift by 40. */
            FRHITextureViewDesc ViewDesc;
            ViewDesc.Texture        = Texture;
            ViewDesc.Range.BaseMip  = 40U;
            ViewDesc.Range.MipCount = 1U;
            const TSharedRef<Tests::FNullTextureView> View =
                MakeShared<Tests::FNullTextureView>( ViewDesc, FRHIObjectId{ 4ULL } );

            FRHIRenderingDesc Desc;
            Desc.Area = { 0, 0, 1U, 1U };
            Desc.ColorAttachments.push_back( { .View       = View,
                                               .LoadOp     = ERHIRenderingLoadOp::Clear,
                                               .StoreOp    = ERHIRenderingStoreOp::Store,
                                               .ClearValue = {} } );
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );

            /** NOTE: Mip 4 of a 16x16 texture is 1x1, the last valid render target. */
            TSharedRef<IRHITexture> Mipped = *Device.CreateTexture( { .Extent         = { 16U, 16U, 1U },
                                                                      .Format         = ERHIFormat::RGBA8Unorm,
                                                                      .MipCount       = 5U,
                                                                      .Usage          = ERHIResourceUsage::RenderTarget,
                                                                      .OptimizedClear = {} } );
            FRHITextureViewDesc LastMipDesc;
            LastMipDesc.Texture       = Mipped;
            LastMipDesc.Range.BaseMip = 4U;
            Desc.ColorAttachments[0].View =
                StaticCastSharedRef<IRHITextureView>( MakeShared<Tests::FNullTextureView>( LastMipDesc, FRHIObjectId{ 5ULL } ) );
            LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        }

        LUMEN_TEST( RHIRenderingDesc, RejectsOverflowingRenderArea )
        {
            Tests::FNullDevice Device;
            TSharedRef<IRHITexture> Texture = *Device.CreateTexture( { .Extent         = { 16U, 16U, 1U },
                                                                       .Format         = ERHIFormat::RGBA8Unorm,
                                                                       .Usage          = ERHIResourceUsage::RenderTarget,
                                                                       .OptimizedClear = {} } );
            FRHITextureViewDesc ViewDesc;
            ViewDesc.Texture = Texture;
            const TSharedRef<Tests::FNullTextureView> View =
                MakeShared<Tests::FNullTextureView>( ViewDesc, FRHIObjectId{ 6ULL } );

            FRHIRenderingDesc Desc;
            Desc.ColorAttachments.push_back( { .View       = View,
                                               .LoadOp     = ERHIRenderingLoadOp::Clear,
                                               .StoreOp    = ERHIRenderingStoreOp::Store,
                                               .ClearValue = {} } );
            /** NOTE: Width wraps to 1 in UInt32 arithmetic; validation must widen before comparing. */
            Desc.Area = { 2, 0, 0xFFFFFFFFU, 1U };
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        }

    } // namespace

} // namespace RHI

} // namespace Lumen
