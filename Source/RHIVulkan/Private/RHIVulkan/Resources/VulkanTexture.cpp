/**
 * @file VulkanTexture.cpp
 * @brief Implements Vulkan image creation, placement and native binding.
 */

#include <RHIVulkan/Resources/VulkanTexture.hpp>

#include <RHIVulkan/Memory/VulkanHeap.hpp>
#include <RHIVulkan/Memory/VulkanMemory.hpp>
#include <RHIVulkan/Types/VulkanObject.hpp>
#include <RHIVulkan/Types/VulkanTypeMapping.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <RHI/Device/RHIBackendCast.hpp>

namespace
{

[[nodiscard]] VkImageType GetVulkanImageType ( const Lumen::RHI::FRHIExtent3D &InExtent ) noexcept
{
    if ( InExtent.Depth > 1U )
    {
        return VK_IMAGE_TYPE_3D;
    }
    if ( InExtent.Height > 1U )
    {
        return VK_IMAGE_TYPE_2D;
    }
    return VK_IMAGE_TYPE_1D;
}

[[nodiscard]] Lumen::RHI::FRHIExpected ValidatePlacedRange ( const Lumen::RHI::FRHITextureDesc &InDesc,
                                                             const Lumen::RHI::IRHIHeap &InHeap,
                                                             const Lumen::RHI::FRHIAllocation &InAllocation ) noexcept
{
    using namespace Lumen;
    if ( InAllocation.Heap != &InHeap or not InAllocation.IsValid() or InAllocation.MemoryClass != InDesc.MemoryClass )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory,
                                                         "Placed texture allocation owner or class is invalid." ) );
    }
    if ( InAllocation.Alignment == 0ULL or ( InAllocation.Alignment & ( InAllocation.Alignment - 1ULL ) ) != 0ULL or
         InAllocation.Offset % InAllocation.Alignment != 0ULL )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Placed texture allocation offset is misaligned." ) );
    }
    const UInt64 HeapSize = InHeap.GetDesc().Size;
    if ( InAllocation.Offset > HeapSize or InAllocation.Size > HeapSize - InAllocation.Offset )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Placed texture allocation lies outside its heap." ) );
    }
    return {};
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanTexture>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanTexture::Create ( TSharedRef<FVulkanDeviceContext> InContext,
                                           const RHI::FRHITextureDesc &InDesc,
                                           RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    const TExpected<VkFormat, RHI::FRHIError> NativeFormat = ToVulkanFormat( InDesc.Format );
    LUMEN_EXPECT( Format, NativeFormat );
    const TExpected<VkSampleCountFlagBits, RHI::FRHIError> NativeSamples = ToVulkanSampleCount( InDesc.SampleCount );
    LUMEN_EXPECT( Samples, NativeSamples );
    const TExpected<VkImageUsageFlags, RHI::FRHIError> NativeUsage = ToVulkanImageUsage( InDesc.Usage );
    LUMEN_EXPECT( Usage, NativeUsage );

    if ( InDesc.Extent.Width > InContext->GetLimits().MaxTextureDimension2D or
         InDesc.Extent.Height > InContext->GetLimits().MaxTextureDimension2D )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Requested texture extent exceeds the published device limit." ) );
    }

    TSharedRef<FVulkanTexture> Texture =
        MakeShared<FVulkanTexture>( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ), InDesc, InId );

    VkImageCreateInfo ImageInfo{};
    ImageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType     = GetVulkanImageType( InDesc.Extent );
    ImageInfo.format        = Format;
    ImageInfo.extent.width  = InDesc.Extent.Width;
    ImageInfo.extent.height = InDesc.Extent.Height;
    ImageInfo.extent.depth  = InDesc.Extent.Depth;
    ImageInfo.mipLevels     = InDesc.MipCount;
    ImageInfo.arrayLayers   = InDesc.LayerCount;
    ImageInfo.samples       = Samples;
    ImageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.usage         = Usage;
    ImageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    LUMEN_VK_TRY( vkCreateImage( Texture->Context->GetDevice(), &ImageInfo, nullptr,
                                 Texture->Image.ResetAndGetAddressOf( FVulkanImageDeleter{ Texture->Context->GetDevice() } ) ),
                  RHI::ERHIErrorCondition::Resource );

    VkMemoryRequirements Requirements{};
    vkGetImageMemoryRequirements( Texture->Context->GetDevice(), Texture->Image.Get(), &Requirements );
    Texture->Requirements.Size      = static_cast<UInt64>( Requirements.size );
    Texture->Requirements.Alignment = static_cast<UInt64>( Requirements.alignment );
    Texture->Requirements.PreferredMemoryClass.Emplace( InDesc.MemoryClass );

    TExpected<FVulkanDeviceMemoryHandle, RHI::FRHIError> Memory = AllocateVulkanMemory(
        Texture->Context->GetPhysicalDevice(), Texture->Context->GetDevice(), Requirements, InDesc.MemoryClass );
    LUMEN_EXPECT( AllocatedMemory, Memory );
    Texture->OwnMemory = static_cast<FVulkanDeviceMemoryHandle &&>( AllocatedMemory );

    LUMEN_VK_TRY( vkBindImageMemory( Texture->Context->GetDevice(), Texture->Image.Get(), Texture->OwnMemory.Get(), 0ULL ),
                  RHI::ERHIErrorCondition::Memory );
    return Texture;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanTexture>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanTexture::CreatePlaced ( TSharedRef<FVulkanDeviceContext> InContext,
                                                 const RHI::FRHITextureDesc &InDesc,
                                                 TSharedRef<RHI::IRHIHeap> InHeap,
                                                 const RHI::FRHIAllocation &InAllocation,
                                                 RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Range, ValidatePlacedRange( InDesc, *InHeap, InAllocation ) );

    const TExpected<VkFormat, RHI::FRHIError> NativeFormat = ToVulkanFormat( InDesc.Format );
    LUMEN_EXPECT( Format, NativeFormat );
    const TExpected<VkSampleCountFlagBits, RHI::FRHIError> NativeSamples = ToVulkanSampleCount( InDesc.SampleCount );
    LUMEN_EXPECT( Samples, NativeSamples );
    const TExpected<VkImageUsageFlags, RHI::FRHIError> NativeUsage = ToVulkanImageUsage( InDesc.Usage );
    LUMEN_EXPECT( Usage, NativeUsage );

    const TOptional<FVulkanHeap *> NativeHeap = RHI::TryRHIBackendCast<FVulkanHeap>( *InHeap );
    if ( not NativeHeap.HasValue() )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Placed texture requires a Vulkan heap." ) );
    }

    TSharedRef<FVulkanTexture> Texture =
        MakeShared<FVulkanTexture>( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ), InDesc, InId );

    VkImageCreateInfo ImageInfo{};
    ImageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType     = GetVulkanImageType( InDesc.Extent );
    ImageInfo.format        = Format;
    ImageInfo.extent.width  = InDesc.Extent.Width;
    ImageInfo.extent.height = InDesc.Extent.Height;
    ImageInfo.extent.depth  = InDesc.Extent.Depth;
    ImageInfo.mipLevels     = InDesc.MipCount;
    ImageInfo.arrayLayers   = InDesc.LayerCount;
    ImageInfo.samples       = Samples;
    ImageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.usage         = Usage;
    ImageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    LUMEN_VK_TRY( vkCreateImage( Texture->Context->GetDevice(), &ImageInfo, nullptr,
                                 Texture->Image.ResetAndGetAddressOf( FVulkanImageDeleter{ Texture->Context->GetDevice() } ) ),
                  RHI::ERHIErrorCondition::Resource );

    VkMemoryRequirements Requirements{};
    vkGetImageMemoryRequirements( Texture->Context->GetDevice(), Texture->Image.Get(), &Requirements );
    if ( InAllocation.Offset % static_cast<UInt64>( Requirements.alignment ) != 0ULL or
         static_cast<UInt64>( Requirements.size ) > InAllocation.Size )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory,
                                                         "Placed texture allocation does not satisfy native requirements." ) );
    }
    if ( ( Requirements.memoryTypeBits & ( 1U << ( *NativeHeap )->GetMemoryTypeIndex() ) ) == 0U )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument(
            RHI::ERHIErrorCondition::Memory, "Placed texture heap is incompatible with native memory requirements." ) );
    }

    Texture->Requirements.Size      = static_cast<UInt64>( Requirements.size );
    Texture->Requirements.Alignment = static_cast<UInt64>( Requirements.alignment );
    Texture->Requirements.PreferredMemoryClass.Emplace( InDesc.MemoryClass );
    Texture->Heap       = static_cast<TSharedPtr<RHI::IRHIHeap>>( InHeap );
    Texture->Allocation = InAllocation;

    LUMEN_VK_TRY( vkBindImageMemory( Texture->Context->GetDevice(), Texture->Image.Get(), ( *NativeHeap )->GetNativeMemory(),
                                     InAllocation.Offset ),
                  RHI::ERHIErrorCondition::Memory );
    return Texture;
}

Lumen::RHIVulkan::FVulkanTexture::FVulkanTexture ( TSharedRef<FVulkanDeviceContext> InContext,
                                                   const RHI::FRHITextureDesc &InDesc,
                                                   RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeVulkanObjectBase<FVulkanTexture>( InId ) ),
      Context( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ) ), Desc( InDesc )
{
    /* */
}

Lumen::RHIVulkan::FVulkanTexture::~FVulkanTexture () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIVulkan, FVulkanTexture )

const Lumen::RHI::FRHITextureDesc &Lumen::RHIVulkan::FVulkanTexture::GetDesc () const noexcept
{
    return Desc;
}

const Lumen::RHI::FRHIResourceRequirements &Lumen::RHIVulkan::FVulkanTexture::GetRequirements () const noexcept
{
    return Requirements;
}

Lumen::RHI::ERHIResourceUsage Lumen::RHIVulkan::FVulkanTexture::GetUsage () const noexcept
{
    return Desc.Usage;
}

Lumen::RHI::ERHIResourceState Lumen::RHIVulkan::FVulkanTexture::GetInitialState () const noexcept
{
    return Desc.InitialState;
}

Lumen::RHI::FRHIResourceLifetime Lumen::RHIVulkan::FVulkanTexture::GetLifetime () const noexcept
{
    return {};
}

VkImage Lumen::RHIVulkan::FVulkanTexture::GetNativeImage () const noexcept
{
    return Image.Get();
}
