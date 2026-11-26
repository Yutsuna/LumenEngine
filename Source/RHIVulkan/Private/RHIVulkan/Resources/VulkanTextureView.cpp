/**
 * @file VulkanTextureView.cpp
 * @brief Implements Vulkan image view creation and parent retention.
 */

#include <RHIVulkan/Resources/VulkanTextureView.hpp>

#include <RHIVulkan/Resources/VulkanTexture.hpp>
#include <RHIVulkan/Types/VulkanObject.hpp>
#include <RHIVulkan/Types/VulkanTypeMapping.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <RHI/Device/RHIBackendCast.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanTextureView>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanTextureView::Create ( TSharedRef<FVulkanDeviceContext> InContext,
                                               const RHI::FRHITextureViewDesc &InDesc,
                                               RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_EXPECT( Validation, InDesc.Validate() );

    const TOptional<FVulkanTexture *> NativeTexture = RHI::TryRHIBackendCast<FVulkanTexture>( *InDesc.Texture );
    if ( not NativeTexture.HasValue() )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Texture view parent is not a Vulkan texture." ) );
    }

    const TExpected<VkImageViewType, RHI::FRHIError> NativeViewType = ToVulkanImageViewType( InDesc.Type );
    LUMEN_EXPECT( ViewType, NativeViewType );

    const RHI::ERHIFormat ViewFormat =
        InDesc.Format == RHI::ERHIFormat::Unknown ? ( *NativeTexture )->GetDesc().Format : InDesc.Format;
    const TExpected<VkFormat, RHI::FRHIError> NativeFormat = ToVulkanFormat( ViewFormat );
    LUMEN_EXPECT( Format, NativeFormat );
    const TExpected<VkImageAspectFlags, RHI::FRHIError> NativeAspect =
        GetVulkanFullImageAspect( ( *NativeTexture )->GetDesc().Format );
    LUMEN_EXPECT( Aspect, NativeAspect );

    TSharedRef<FVulkanTextureView> View = MakeShared<FVulkanTextureView>(
        static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ), InDesc, InDesc.Texture, InId );

    VkImageViewCreateInfo ViewInfo{};
    ViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ViewInfo.image                           = ( *NativeTexture )->GetNativeImage();
    ViewInfo.viewType                        = ViewType;
    ViewInfo.format                          = Format;
    ViewInfo.subresourceRange.aspectMask     = Aspect;
    ViewInfo.subresourceRange.baseMipLevel   = InDesc.Range.BaseMip;
    ViewInfo.subresourceRange.levelCount     = InDesc.Range.MipCount;
    ViewInfo.subresourceRange.baseArrayLayer = InDesc.Range.BaseLayer;
    ViewInfo.subresourceRange.layerCount     = InDesc.Range.LayerCount;
    LUMEN_VK_TRY( vkCreateImageView( View->Context->GetDevice(), &ViewInfo, nullptr,
                                     View->View.ResetAndGetAddressOf( FVulkanImageViewDeleter{ View->Context->GetDevice() } ) ),
                  RHI::ERHIErrorCondition::Resource );
    return View;
}

Lumen::RHIVulkan::FVulkanTextureView::FVulkanTextureView ( TSharedRef<FVulkanDeviceContext> InContext,
                                                           const RHI::FRHITextureViewDesc &InDesc,
                                                           TSharedPtr<RHI::IRHITexture> InTexture,
                                                           RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeVulkanObjectBase<FVulkanTextureView>( InId ) ),
      Context( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ) ), Desc( InDesc ),
      Texture( static_cast<TSharedPtr<RHI::IRHITexture> &&>( InTexture ) )
{
    /* */
}

Lumen::RHIVulkan::FVulkanTextureView::~FVulkanTextureView () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIVulkan, FVulkanTextureView )

const Lumen::RHI::FRHITextureViewDesc &Lumen::RHIVulkan::FVulkanTextureView::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TSharedPtr<Lumen::RHI::IRHITexture> Lumen::RHIVulkan::FVulkanTextureView::GetTexture () const noexcept
{
    return Texture;
}
