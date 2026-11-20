/**
 * @file VulkanTypeMapping.cpp
 * @brief Implements portable RHI-to-Vulkan enum, format, usage, and state mappings.
 */

#include "VulkanTypeMapping.hpp"
#include "RHI/Types/RHIFormatInfo.hpp"

/**
 * Private
 */

namespace
{

[[nodiscard]] Lumen::TExpected<VkFormat, Lumen::RHI::FRHIError> MapFormatInternal ( Lumen::RHI::ERHIFormat InFormat,
                                                                                    Lumen::Bool bAllowUnknown ) noexcept
{
    using Lumen::RHI::ERHIFormat;

    switch ( InFormat )
    {
    case ERHIFormat::Unknown:
        if ( bAllowUnknown )
        {
            return VK_FORMAT_UNDEFINED;
        }
        return Lumen::TUnexpected( Lumen::RHI::MakeRHIInvalidResource( "ERHIFormat::Unknown is not a valid concrete format." ) );
    case ERHIFormat::R8Unorm:
        return VK_FORMAT_R8_UNORM;
    case ERHIFormat::R8Snorm:
        return VK_FORMAT_R8_SNORM;
    case ERHIFormat::R8Uint:
        return VK_FORMAT_R8_UINT;
    case ERHIFormat::R8Sint:
        return VK_FORMAT_R8_SINT;
    case ERHIFormat::RG8Unorm:
        return VK_FORMAT_R8G8_UNORM;
    case ERHIFormat::RG8Snorm:
        return VK_FORMAT_R8G8_SNORM;
    case ERHIFormat::RG8Uint:
        return VK_FORMAT_R8G8_UINT;
    case ERHIFormat::RG8Sint:
        return VK_FORMAT_R8G8_SINT;
    case ERHIFormat::RGBA8Unorm:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case ERHIFormat::RGBA8Snorm:
        return VK_FORMAT_R8G8B8A8_SNORM;
    case ERHIFormat::RGBA8Uint:
        return VK_FORMAT_R8G8B8A8_UINT;
    case ERHIFormat::RGBA8Sint:
        return VK_FORMAT_R8G8B8A8_SINT;
    case ERHIFormat::RGBA8Srgb:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case ERHIFormat::BGRA8Unorm:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case ERHIFormat::BGRA8Srgb:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case ERHIFormat::R16Unorm:
        return VK_FORMAT_R16_UNORM;
    case ERHIFormat::R16Snorm:
        return VK_FORMAT_R16_SNORM;
    case ERHIFormat::R16Uint:
        return VK_FORMAT_R16_UINT;
    case ERHIFormat::R16Sint:
        return VK_FORMAT_R16_SINT;
    case ERHIFormat::R16Float:
        return VK_FORMAT_R16_SFLOAT;
    case ERHIFormat::RG16Unorm:
        return VK_FORMAT_R16G16_UNORM;
    case ERHIFormat::RG16Snorm:
        return VK_FORMAT_R16G16_SNORM;
    case ERHIFormat::RG16Uint:
        return VK_FORMAT_R16G16_UINT;
    case ERHIFormat::RG16Sint:
        return VK_FORMAT_R16G16_SINT;
    case ERHIFormat::RG16Float:
        return VK_FORMAT_R16G16_SFLOAT;
    case ERHIFormat::RGBA16Unorm:
        return VK_FORMAT_R16G16B16A16_UNORM;
    case ERHIFormat::RGBA16Snorm:
        return VK_FORMAT_R16G16B16A16_SNORM;
    case ERHIFormat::RGBA16Uint:
        return VK_FORMAT_R16G16B16A16_UINT;
    case ERHIFormat::RGBA16Sint:
        return VK_FORMAT_R16G16B16A16_SINT;
    case ERHIFormat::RGBA16Float:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case ERHIFormat::R32Uint:
        return VK_FORMAT_R32_UINT;
    case ERHIFormat::R32Sint:
        return VK_FORMAT_R32_SINT;
    case ERHIFormat::R32Float:
        return VK_FORMAT_R32_SFLOAT;
    case ERHIFormat::RG32Uint:
        return VK_FORMAT_R32G32_UINT;
    case ERHIFormat::RG32Sint:
        return VK_FORMAT_R32G32_SINT;
    case ERHIFormat::RG32Float:
        return VK_FORMAT_R32G32_SFLOAT;
    case ERHIFormat::RGB32Uint:
        return VK_FORMAT_R32G32B32_UINT;
    case ERHIFormat::RGB32Sint:
        return VK_FORMAT_R32G32B32_SINT;
    case ERHIFormat::RGB32Float:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case ERHIFormat::RGBA32Uint:
        return VK_FORMAT_R32G32B32A32_UINT;
    case ERHIFormat::RGBA32Sint:
        return VK_FORMAT_R32G32B32A32_SINT;
    case ERHIFormat::RGBA32Float:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case ERHIFormat::D16Unorm:
        return VK_FORMAT_D16_UNORM;
    case ERHIFormat::D24UnormS8Uint:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case ERHIFormat::D32Float:
        return VK_FORMAT_D32_SFLOAT;
    case ERHIFormat::D32FloatS8Uint:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case ERHIFormat::BC1RGBAUnorm:
        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case ERHIFormat::BC1RGBASrgb:
        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case ERHIFormat::BC3RGBAUnorm:
        return VK_FORMAT_BC3_UNORM_BLOCK;
    case ERHIFormat::BC3RGBASrgb:
        return VK_FORMAT_BC3_SRGB_BLOCK;
    case ERHIFormat::BC7RGBAUnorm:
        return VK_FORMAT_BC7_UNORM_BLOCK;
    case ERHIFormat::BC7RGBASrgb:
        return VK_FORMAT_BC7_SRGB_BLOCK;
    default:
        return Lumen::TUnexpected( Lumen::RHI::MakeRHIInvalidResource( "Unrecognized ERHIFormat value." ) );
    }
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<VkFormat, Lumen::RHI::FRHIError> Lumen::RHIVulkan::ToVulkanFormat ( RHI::ERHIFormat InFormat ) noexcept
{
    return MapFormatInternal( InFormat, false );
}

Lumen::TExpected<VkFormat, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanFormatOrUndefined ( RHI::ERHIFormat InFormat ) noexcept
{
    return MapFormatInternal( InFormat, true );
}

Lumen::RHI::ERHIFormat Lumen::RHIVulkan::ToRHIFormat ( VkFormat InFormat ) noexcept
{
    using RHI::ERHIFormat;

    switch ( InFormat )
    {
    case VK_FORMAT_R8_UNORM:
        return ERHIFormat::R8Unorm;
    case VK_FORMAT_R8_SNORM:
        return ERHIFormat::R8Snorm;
    case VK_FORMAT_R8_UINT:
        return ERHIFormat::R8Uint;
    case VK_FORMAT_R8_SINT:
        return ERHIFormat::R8Sint;
    case VK_FORMAT_R8G8_UNORM:
        return ERHIFormat::RG8Unorm;
    case VK_FORMAT_R8G8_SNORM:
        return ERHIFormat::RG8Snorm;
    case VK_FORMAT_R8G8_UINT:
        return ERHIFormat::RG8Uint;
    case VK_FORMAT_R8G8_SINT:
        return ERHIFormat::RG8Sint;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return ERHIFormat::RGBA8Unorm;
    case VK_FORMAT_R8G8B8A8_SNORM:
        return ERHIFormat::RGBA8Snorm;
    case VK_FORMAT_R8G8B8A8_UINT:
        return ERHIFormat::RGBA8Uint;
    case VK_FORMAT_R8G8B8A8_SINT:
        return ERHIFormat::RGBA8Sint;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return ERHIFormat::RGBA8Srgb;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return ERHIFormat::BGRA8Unorm;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return ERHIFormat::BGRA8Srgb;
    case VK_FORMAT_R16_UNORM:
        return ERHIFormat::R16Unorm;
    case VK_FORMAT_R16_SNORM:
        return ERHIFormat::R16Snorm;
    case VK_FORMAT_R16_UINT:
        return ERHIFormat::R16Uint;
    case VK_FORMAT_R16_SINT:
        return ERHIFormat::R16Sint;
    case VK_FORMAT_R16_SFLOAT:
        return ERHIFormat::R16Float;
    case VK_FORMAT_R16G16_UNORM:
        return ERHIFormat::RG16Unorm;
    case VK_FORMAT_R16G16_SNORM:
        return ERHIFormat::RG16Snorm;
    case VK_FORMAT_R16G16_UINT:
        return ERHIFormat::RG16Uint;
    case VK_FORMAT_R16G16_SINT:
        return ERHIFormat::RG16Sint;
    case VK_FORMAT_R16G16_SFLOAT:
        return ERHIFormat::RG16Float;
    case VK_FORMAT_R16G16B16A16_UNORM:
        return ERHIFormat::RGBA16Unorm;
    case VK_FORMAT_R16G16B16A16_SNORM:
        return ERHIFormat::RGBA16Snorm;
    case VK_FORMAT_R16G16B16A16_UINT:
        return ERHIFormat::RGBA16Uint;
    case VK_FORMAT_R16G16B16A16_SINT:
        return ERHIFormat::RGBA16Sint;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return ERHIFormat::RGBA16Float;
    case VK_FORMAT_R32_UINT:
        return ERHIFormat::R32Uint;
    case VK_FORMAT_R32_SINT:
        return ERHIFormat::R32Sint;
    case VK_FORMAT_R32_SFLOAT:
        return ERHIFormat::R32Float;
    case VK_FORMAT_R32G32_UINT:
        return ERHIFormat::RG32Uint;
    case VK_FORMAT_R32G32_SINT:
        return ERHIFormat::RG32Sint;
    case VK_FORMAT_R32G32_SFLOAT:
        return ERHIFormat::RG32Float;
    case VK_FORMAT_R32G32B32_UINT:
        return ERHIFormat::RGB32Uint;
    case VK_FORMAT_R32G32B32_SINT:
        return ERHIFormat::RGB32Sint;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return ERHIFormat::RGB32Float;
    case VK_FORMAT_R32G32B32A32_UINT:
        return ERHIFormat::RGBA32Uint;
    case VK_FORMAT_R32G32B32A32_SINT:
        return ERHIFormat::RGBA32Sint;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return ERHIFormat::RGBA32Float;
    case VK_FORMAT_D16_UNORM:
        return ERHIFormat::D16Unorm;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return ERHIFormat::D24UnormS8Uint;
    case VK_FORMAT_D32_SFLOAT:
        return ERHIFormat::D32Float;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return ERHIFormat::D32FloatS8Uint;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        return ERHIFormat::BC1RGBAUnorm;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        return ERHIFormat::BC1RGBASrgb;
    case VK_FORMAT_BC3_UNORM_BLOCK:
        return ERHIFormat::BC3RGBAUnorm;
    case VK_FORMAT_BC3_SRGB_BLOCK:
        return ERHIFormat::BC3RGBASrgb;
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return ERHIFormat::BC7RGBAUnorm;
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return ERHIFormat::BC7RGBASrgb;
    default:
        return ERHIFormat::Unknown;
    }
}

Lumen::TExpected<VkImageAspectFlags, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanImageAspect ( RHI::ERHIFormatAspect InAspect ) noexcept
{
    constexpr UInt8 ValidMask =
        static_cast<UInt8>( RHI::ERHIFormatAspect::Color | RHI::ERHIFormatAspect::Depth | RHI::ERHIFormatAspect::Stencil );
    const UInt8 Raw = static_cast<UInt8>( InAspect );
    if ( Raw == 0U or ( Raw & ~ValidMask ) != 0U )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Invalid ERHIFormatAspect bitmask." ) );
    }

    VkImageAspectFlags Flags = 0U;
    if ( HasAnyFlags( InAspect, RHI::ERHIFormatAspect::Color ) )
    {
        Flags |= VK_IMAGE_ASPECT_COLOR_BIT;
    }
    if ( HasAnyFlags( InAspect, RHI::ERHIFormatAspect::Depth ) )
    {
        Flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if ( HasAnyFlags( InAspect, RHI::ERHIFormatAspect::Stencil ) )
    {
        Flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    return Flags;
}

Lumen::TExpected<VkImageAspectFlags, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::GetVulkanFullImageAspect ( RHI::ERHIFormat InFormat ) noexcept
{
    const TOptional<RHI::FRHIFormatInfo> Info = RHI::TryGetRHIFormatInfo( InFormat );

    if ( not Info or Info->Aspects == RHI::ERHIFormatAspect::None ) [[unlikely]]
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Cannot derive Vulkan image aspects for Unknown or invalid format." ) );
    }
    return ToVulkanImageAspect( Info->Aspects );
}

Lumen::TExpected<VkBufferUsageFlags, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanBufferUsage ( RHI::ERHIResourceUsage InUsage ) noexcept
{
    if ( not RHI::IsValidRHIResourceUsage( InUsage ) or InUsage == RHI::ERHIResourceUsage::None ) [[unlikely]]
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Buffer usage flags must be valid and nonzero." ) );
    }

    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::RenderTarget | RHI::ERHIResourceUsage::DepthStencil ) ) [[unlikely]]
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "RenderTarget and DepthStencil usages are invalid for buffers." ) );
    }

    VkBufferUsageFlags Flags = 0U;
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::CopySource ) )
    {
        Flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::CopyDestination | RHI::ERHIResourceUsage::QueryResolve ) )
    {
        Flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::VertexBuffer ) )
    {
        Flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::IndexBuffer ) )
    {
        Flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::ConstantBuffer ) )
    {
        Flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::ShaderResource | RHI::ERHIResourceUsage::UnorderedAccess ) )
    {
        Flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::IndirectArgument ) )
    {
        Flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }

    return Flags;
}

Lumen::TExpected<VkImageUsageFlags, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanImageUsage ( RHI::ERHIResourceUsage InUsage ) noexcept
{
    if ( not RHI::IsValidRHIResourceUsage( InUsage ) or InUsage == RHI::ERHIResourceUsage::None ) [[unlikely]]
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Texture usage flags must be valid and nonzero." ) );
    }

    constexpr RHI::ERHIResourceUsage BufferOnlyUsages =
        RHI::ERHIResourceUsage::VertexBuffer | RHI::ERHIResourceUsage::IndexBuffer | RHI::ERHIResourceUsage::ConstantBuffer |
        RHI::ERHIResourceUsage::IndirectArgument | RHI::ERHIResourceUsage::QueryResolve;

    if ( HasAnyFlags( InUsage, BufferOnlyUsages ) ) [[unlikely]]
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Buffer-only usage flags are invalid for textures." ) );
    }

    VkImageUsageFlags Flags = 0U;
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::CopySource ) )
    {
        Flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::CopyDestination ) )
    {
        Flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::ShaderResource ) )
    {
        Flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::UnorderedAccess ) )
    {
        Flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::RenderTarget ) )
    {
        Flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::DepthStencil ) )
    {
        Flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    return Flags;
}

Lumen::TExpected<Lumen::RHIVulkan::FVulkanResourceBarrierState, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanBufferState ( RHI::ERHIResourceState InState ) noexcept
{
    if ( not RHI::IsValidRHIResourceState( InState ) ) [[unlikely]]
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Unknown ERHIResourceState bits." ) );
    }

    constexpr RHI::ERHIResourceState ImageOnlyStates = RHI::ERHIResourceState::RenderTarget | RHI::ERHIResourceState::DepthWrite |
                                                       RHI::ERHIResourceState::DepthRead | RHI::ERHIResourceState::Present;

    if ( HasAnyFlags( InState, ImageOnlyStates ) ) [[unlikely]]
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Image-only resource state requested for a buffer." ) );
    }

    FVulkanResourceBarrierState Result{};
    if ( InState == RHI::ERHIResourceState::Undefined )
    {
        return Result;
    }

    if ( HasAnyFlags( InState, RHI::ERHIResourceState::Common ) )
    {
        Result.StageMask |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        Result.AccessMask |= VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    }
    if ( HasAnyFlags( InState, RHI::ERHIResourceState::VertexBuffer ) )
    {
        Result.StageMask |= VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
        Result.AccessMask |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if ( HasAnyFlags( InState, RHI::ERHIResourceState::IndexBuffer ) )
    {
        Result.StageMask |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
        Result.AccessMask |= VK_ACCESS_2_INDEX_READ_BIT;
    }
    if ( HasAnyFlags( InState, RHI::ERHIResourceState::ConstantBuffer ) )
    {
        Result.StageMask |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        Result.AccessMask |= VK_ACCESS_2_UNIFORM_READ_BIT;
    }
    if ( HasAnyFlags( InState, RHI::ERHIResourceState::ShaderResource ) )
    {
        Result.StageMask |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        Result.AccessMask |= VK_ACCESS_2_SHADER_READ_BIT;
    }
    if ( HasAnyFlags( InState, RHI::ERHIResourceState::UnorderedAccess ) )
    {
        Result.StageMask |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        Result.AccessMask |= VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
    }
    if ( HasAnyFlags( InState, RHI::ERHIResourceState::CopySource ) )
    {
        Result.StageMask |= VK_PIPELINE_STAGE_2_COPY_BIT;
        Result.AccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT;
    }
    if ( HasAnyFlags( InState, RHI::ERHIResourceState::CopyDestination ) )
    {
        Result.StageMask |= VK_PIPELINE_STAGE_2_COPY_BIT;
        Result.AccessMask |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    if ( HasAnyFlags( InState, RHI::ERHIResourceState::ResolveSource ) )
    {
        Result.StageMask |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;
        Result.AccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT;
    }
    if ( HasAnyFlags( InState, RHI::ERHIResourceState::ResolveDestination ) )
    {
        Result.StageMask |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;
        Result.AccessMask |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    if ( HasAnyFlags( InState, RHI::ERHIResourceState::IndirectArgument ) )
    {
        Result.StageMask |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
        Result.AccessMask |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    }

    return Result;
}

Lumen::TExpected<Lumen::RHIVulkan::FVulkanResourceBarrierState, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanImageState ( RHI::ERHIResourceState InState, Bool bIsDepthStencil ) noexcept
{
    if ( not RHI::IsValidRHIResourceState( InState ) ) [[unlikely]]
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Unknown ERHIResourceState bits." ) );
    }

    constexpr RHI::ERHIResourceState BufferOnlyStates =
        RHI::ERHIResourceState::VertexBuffer | RHI::ERHIResourceState::IndexBuffer | RHI::ERHIResourceState::ConstantBuffer |
        RHI::ERHIResourceState::IndirectArgument;

    if ( HasAnyFlags( InState, BufferOnlyStates ) ) [[unlikely]]
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Buffer-only resource state requested for a texture." ) );
    }

    FVulkanResourceBarrierState Result{};
    if ( InState == RHI::ERHIResourceState::Undefined )
    {
        Result.ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        return Result;
    }

    if ( InState == RHI::ERHIResourceState::Common )
    {
        Result.StageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        Result.AccessMask  = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        Result.ImageLayout = VK_IMAGE_LAYOUT_GENERAL;
        return Result;
    }

    if ( InState == RHI::ERHIResourceState::Present )
    {
        Result.StageMask   = VK_PIPELINE_STAGE_2_NONE;
        Result.AccessMask  = VK_ACCESS_2_NONE;
        Result.ImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        return Result;
    }

    if ( InState == RHI::ERHIResourceState::RenderTarget )
    {
        if ( bIsDepthStencil ) [[unlikely]]
        {
            return TUnexpected(
                RHI::MakeRHIInvalidResource( "RenderTarget state is incompatible with depth/stencil textures." ) );
        }
        Result.StageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        Result.AccessMask  = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        Result.ImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        return Result;
    }

    if ( InState == RHI::ERHIResourceState::DepthWrite )
    {
        if ( not bIsDepthStencil ) [[unlikely]]
        {
            return TUnexpected( RHI::MakeRHIInvalidResource( "DepthWrite state requires a depth/stencil texture." ) );
        }
        Result.StageMask   = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        Result.AccessMask  = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        Result.ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        return Result;
    }

    if ( InState == RHI::ERHIResourceState::UnorderedAccess )
    {
        Result.StageMask   = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        Result.AccessMask  = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
        Result.ImageLayout = VK_IMAGE_LAYOUT_GENERAL;
        return Result;
    }

    constexpr RHI::ERHIResourceState ReadOnlyImageMask =
        RHI::ERHIResourceState::ShaderResource | RHI::ERHIResourceState::DepthRead;
    if ( ( static_cast<UInt32>( InState ) & ~static_cast<UInt32>( ReadOnlyImageMask ) ) == 0U )
    {
        if ( HasAnyFlags( InState, RHI::ERHIResourceState::DepthRead ) )
        {
            if ( not bIsDepthStencil ) [[unlikely]]
            {
                return TUnexpected( RHI::MakeRHIInvalidResource( "DepthRead state requires a depth/stencil texture." ) );
            }
            Result.StageMask |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            Result.AccessMask |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        }
        if ( HasAnyFlags( InState, RHI::ERHIResourceState::ShaderResource ) )
        {
            Result.StageMask |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            Result.AccessMask |= VK_ACCESS_2_SHADER_READ_BIT;
        }
        Result.ImageLayout =
            bIsDepthStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return Result;
    }

    constexpr RHI::ERHIResourceState TransferSrcMask = RHI::ERHIResourceState::CopySource | RHI::ERHIResourceState::ResolveSource;
    if ( ( static_cast<UInt32>( InState ) & ~static_cast<UInt32>( TransferSrcMask ) ) == 0U )
    {
        if ( HasAnyFlags( InState, RHI::ERHIResourceState::CopySource ) )
        {
            Result.StageMask |= VK_PIPELINE_STAGE_2_COPY_BIT;
        }
        if ( HasAnyFlags( InState, RHI::ERHIResourceState::ResolveSource ) )
        {
            Result.StageMask |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;
        }
        Result.AccessMask  = VK_ACCESS_2_TRANSFER_READ_BIT;
        Result.ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        return Result;
    }

    constexpr RHI::ERHIResourceState TransferDstMask =
        RHI::ERHIResourceState::CopyDestination | RHI::ERHIResourceState::ResolveDestination;
    if ( ( static_cast<UInt32>( InState ) & ~static_cast<UInt32>( TransferDstMask ) ) == 0U )
    {
        if ( HasAnyFlags( InState, RHI::ERHIResourceState::CopyDestination ) )
        {
            Result.StageMask |= VK_PIPELINE_STAGE_2_COPY_BIT;
        }
        if ( HasAnyFlags( InState, RHI::ERHIResourceState::ResolveDestination ) )
        {
            Result.StageMask |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;
        }
        Result.AccessMask  = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        Result.ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        return Result;
    }

    return TUnexpected(
        RHI::MakeRHIInvalidResource( "Combined ERHIResourceState flags require incompatible Vulkan image layouts." ) );
}

Lumen::TExpected<VkSampleCountFlagBits, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanSampleCount ( RHI::ERHISampleCount InCount ) noexcept
{
    switch ( InCount )
    {
    case RHI::ERHISampleCount::Count1:
        return VK_SAMPLE_COUNT_1_BIT;
    case RHI::ERHISampleCount::Count2:
        return VK_SAMPLE_COUNT_2_BIT;
    case RHI::ERHISampleCount::Count4:
        return VK_SAMPLE_COUNT_4_BIT;
    case RHI::ERHISampleCount::Count8:
        return VK_SAMPLE_COUNT_8_BIT;
    case RHI::ERHISampleCount::Count16:
        return VK_SAMPLE_COUNT_16_BIT;
    case RHI::ERHISampleCount::Count32:
        return VK_SAMPLE_COUNT_32_BIT;
    case RHI::ERHISampleCount::Count64:
        return VK_SAMPLE_COUNT_64_BIT;
    default:
        return TUnexpected( RHI::MakeRHIInvalidPipeline( "Invalid ERHISampleCount value." ) );
    }
}

Lumen::TExpected<VkShaderStageFlags, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanShaderStageFlags ( RHI::ERHIShaderStage InStages ) noexcept
{
    const UInt32 Raw     = static_cast<UInt32>( InStages );
    const UInt32 AllMask = static_cast<UInt32>( RHI::ERHIShaderStage::All );
    if ( Raw == 0U or ( Raw & ~AllMask ) != 0U )
    {
        return TUnexpected( RHI::MakeRHIInvalidShader( "Invalid or empty ERHIShaderStage mask." ) );
    }

    if ( InStages == RHI::ERHIShaderStage::All )
    {
        return VK_SHADER_STAGE_ALL;
    }
    if ( InStages == RHI::ERHIShaderStage::AllGraphics )
    {
        return VK_SHADER_STAGE_ALL_GRAPHICS;
    }

    VkShaderStageFlags Flags = 0U;
    if ( HasAnyFlags( InStages, RHI::ERHIShaderStage::Vertex ) )
    {
        Flags |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if ( HasAnyFlags( InStages, RHI::ERHIShaderStage::Pixel ) )
    {
        Flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if ( HasAnyFlags( InStages, RHI::ERHIShaderStage::Compute ) )
    {
        Flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    }
    if ( HasAnyFlags( InStages, RHI::ERHIShaderStage::Geometry ) )
    {
        Flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
    }
    if ( HasAnyFlags( InStages, RHI::ERHIShaderStage::Hull ) )
    {
        Flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    }
    if ( HasAnyFlags( InStages, RHI::ERHIShaderStage::Domain ) )
    {
        Flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    }
    if ( HasAnyFlags( InStages, RHI::ERHIShaderStage::Amplification ) )
    {
        Flags |= VK_SHADER_STAGE_TASK_BIT_EXT;
    }
    if ( HasAnyFlags( InStages, RHI::ERHIShaderStage::Mesh ) )
    {
        Flags |= VK_SHADER_STAGE_MESH_BIT_EXT;
    }

    return Flags;
}

Lumen::TExpected<VkShaderStageFlagBits, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanSingleShaderStage ( RHI::ERHIShaderStage InStage ) noexcept
{
    switch ( InStage )
    {
    case RHI::ERHIShaderStage::Vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case RHI::ERHIShaderStage::Pixel:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case RHI::ERHIShaderStage::Compute:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    case RHI::ERHIShaderStage::Geometry:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case RHI::ERHIShaderStage::Hull:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case RHI::ERHIShaderStage::Domain:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case RHI::ERHIShaderStage::Amplification:
        return VK_SHADER_STAGE_TASK_BIT_EXT;
    case RHI::ERHIShaderStage::Mesh:
        return VK_SHADER_STAGE_MESH_BIT_EXT;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidShader( "ERHIShaderStage must specify a single valid stage." ) );
    }
}

Lumen::TExpected<VkDescriptorType, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanDescriptorType ( RHI::ERHIBindingType InType ) noexcept
{
    switch ( InType )
    {
    case RHI::ERHIBindingType::ConstantBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case RHI::ERHIBindingType::ReadOnlyBuffer:
    case RHI::ERHIBindingType::ReadWriteBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case RHI::ERHIBindingType::Texture:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case RHI::ERHIBindingType::StorageTexture:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case RHI::ERHIBindingType::Sampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case RHI::ERHIBindingType::CombinedTextureSampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidBinding( "Invalid ERHIBindingType value." ) );
    }
}

Lumen::TExpected<VkIndexType, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanIndexType ( RHI::ERHIIndexFormat InFormat ) noexcept
{
    switch ( InFormat )
    {
    case RHI::ERHIIndexFormat::Index16:
        return VK_INDEX_TYPE_UINT16;
    case RHI::ERHIIndexFormat::Index32:
        return VK_INDEX_TYPE_UINT32;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidCommand( "Invalid ERHIIndexFormat value." ) );
    }
}

Lumen::TExpected<VkImageViewType, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanImageViewType ( RHI::ERHITextureViewType InType ) noexcept
{
    switch ( InType )
    {
    case RHI::ERHITextureViewType::Texture1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case RHI::ERHITextureViewType::Texture2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case RHI::ERHITextureViewType::Texture3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case RHI::ERHITextureViewType::TextureCube:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case RHI::ERHITextureViewType::Texture2DArray:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidResource( "Invalid ERHITextureViewType value." ) );
    }
}

Lumen::TExpected<VkPrimitiveTopology, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanPrimitiveTopology ( RHI::ERHIPrimitiveTopology InTopology ) noexcept
{
    switch ( InTopology )
    {
    case RHI::ERHIPrimitiveTopology::PointList:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case RHI::ERHIPrimitiveTopology::LineList:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case RHI::ERHIPrimitiveTopology::LineStrip:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case RHI::ERHIPrimitiveTopology::TriangleList:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case RHI::ERHIPrimitiveTopology::TriangleStrip:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case RHI::ERHIPrimitiveTopology::PatchList:
        return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidPipeline( "Invalid ERHIPrimitiveTopology value." ) );
    }
}

Lumen::TExpected<VkPolygonMode, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanPolygonMode ( RHI::ERHIFillMode InFillMode ) noexcept
{
    switch ( InFillMode )
    {
    case RHI::ERHIFillMode::Solid:
        return VK_POLYGON_MODE_FILL;
    case RHI::ERHIFillMode::Wireframe:
        return VK_POLYGON_MODE_LINE;
    case RHI::ERHIFillMode::Point:
        return VK_POLYGON_MODE_POINT;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidPipeline( "Invalid ERHIFillMode value." ) );
    }
}

Lumen::TExpected<VkCullModeFlags, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanCullMode ( RHI::ERHICullMode InCullMode ) noexcept
{
    switch ( InCullMode )
    {
    case RHI::ERHICullMode::None:
        return VK_CULL_MODE_NONE;
    case RHI::ERHICullMode::Front:
        return VK_CULL_MODE_FRONT_BIT;
    case RHI::ERHICullMode::Back:
        return VK_CULL_MODE_BACK_BIT;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidPipeline( "Invalid ERHICullMode value." ) );
    }
}

Lumen::TExpected<VkFrontFace, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanFrontFace ( RHI::ERHIFrontFace InFrontFace ) noexcept
{
    switch ( InFrontFace )
    {
    case RHI::ERHIFrontFace::CounterClockwise:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    case RHI::ERHIFrontFace::Clockwise:
        return VK_FRONT_FACE_CLOCKWISE;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidPipeline( "Invalid ERHIFrontFace value." ) );
    }
}

Lumen::TExpected<VkBlendFactor, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanBlendFactor ( RHI::ERHIBlendFactor InFactor ) noexcept
{
    switch ( InFactor )
    {
    case RHI::ERHIBlendFactor::Zero:
        return VK_BLEND_FACTOR_ZERO;
    case RHI::ERHIBlendFactor::One:
        return VK_BLEND_FACTOR_ONE;
    case RHI::ERHIBlendFactor::SourceAlpha:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case RHI::ERHIBlendFactor::InverseSourceAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case RHI::ERHIBlendFactor::DestinationColor:
        return VK_BLEND_FACTOR_DST_COLOR;
    case RHI::ERHIBlendFactor::InverseDestinationColor:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidPipeline( "Invalid ERHIBlendFactor value." ) );
    }
}

Lumen::TExpected<VkBlendOp, Lumen::RHI::FRHIError> Lumen::RHIVulkan::ToVulkanBlendOp ( RHI::ERHIBlendOp InOp ) noexcept
{
    switch ( InOp )
    {
    case RHI::ERHIBlendOp::Add:
        return VK_BLEND_OP_ADD;
    case RHI::ERHIBlendOp::Subtract:
        return VK_BLEND_OP_SUBTRACT;
    case RHI::ERHIBlendOp::ReverseSubtract:
        return VK_BLEND_OP_REVERSE_SUBTRACT;
    case RHI::ERHIBlendOp::Min:
        return VK_BLEND_OP_MIN;
    case RHI::ERHIBlendOp::Max:
        return VK_BLEND_OP_MAX;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidPipeline( "Invalid ERHIBlendOp value." ) );
    }
}

Lumen::TExpected<VkStencilOp, Lumen::RHI::FRHIError> Lumen::RHIVulkan::ToVulkanStencilOp ( RHI::ERHIStencilOp InOp ) noexcept
{
    switch ( InOp )
    {
    case RHI::ERHIStencilOp::Keep:
        return VK_STENCIL_OP_KEEP;
    case RHI::ERHIStencilOp::Zero:
        return VK_STENCIL_OP_ZERO;
    case RHI::ERHIStencilOp::Replace:
        return VK_STENCIL_OP_REPLACE;
    case RHI::ERHIStencilOp::IncrementClamp:
        return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case RHI::ERHIStencilOp::DecrementClamp:
        return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case RHI::ERHIStencilOp::Invert:
        return VK_STENCIL_OP_INVERT;
    case RHI::ERHIStencilOp::IncrementWrap:
        return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case RHI::ERHIStencilOp::DecrementWrap:
        return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidPipeline( "Invalid ERHIStencilOp value." ) );
    }
}

Lumen::TExpected<VkCompareOp, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanCompareOp ( RHI::ERHICompareFunction InCompare ) noexcept
{
    switch ( InCompare )
    {
    case RHI::ERHICompareFunction::Never:
        return VK_COMPARE_OP_NEVER;
    case RHI::ERHICompareFunction::Less:
        return VK_COMPARE_OP_LESS;
    case RHI::ERHICompareFunction::Equal:
        return VK_COMPARE_OP_EQUAL;
    case RHI::ERHICompareFunction::LessEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case RHI::ERHICompareFunction::Greater:
        return VK_COMPARE_OP_GREATER;
    case RHI::ERHICompareFunction::NotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case RHI::ERHICompareFunction::GreaterEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case RHI::ERHICompareFunction::Always:
        return VK_COMPARE_OP_ALWAYS;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidPipeline( "Invalid ERHICompareFunction value." ) );
    }
}

Lumen::TExpected<VkFilter, Lumen::RHI::FRHIError> Lumen::RHIVulkan::ToVulkanFilter ( RHI::ERHISamplerFilter InFilter ) noexcept
{
    switch ( InFilter )
    {
    case RHI::ERHISamplerFilter::Nearest:
        return VK_FILTER_NEAREST;
    case RHI::ERHISamplerFilter::Linear:
        return VK_FILTER_LINEAR;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidResource( "Invalid ERHISamplerFilter value." ) );
    }
}

Lumen::TExpected<VkSamplerMipmapMode, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanMipmapMode ( RHI::ERHISamplerFilter InFilter ) noexcept
{
    switch ( InFilter )
    {
    case RHI::ERHISamplerFilter::Nearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case RHI::ERHISamplerFilter::Linear:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidResource( "Invalid ERHISamplerFilter value." ) );
    }
}

Lumen::TExpected<VkSamplerAddressMode, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanAddressMode ( RHI::ERHISamplerAddressMode InMode ) noexcept
{
    switch ( InMode )
    {
    case RHI::ERHISamplerAddressMode::Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case RHI::ERHISamplerAddressMode::MirroredRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case RHI::ERHISamplerAddressMode::ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case RHI::ERHISamplerAddressMode::ClampToBorder:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidResource( "Invalid ERHISamplerAddressMode value." ) );
    }
}

Lumen::TExpected<VkAttachmentLoadOp, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanAttachmentLoadOp ( RHI::ERHIRenderingLoadOp InOp ) noexcept
{
    switch ( InOp )
    {
    case RHI::ERHIRenderingLoadOp::Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case RHI::ERHIRenderingLoadOp::Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case RHI::ERHIRenderingLoadOp::DontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidCommand( "Invalid ERHIRenderingLoadOp value." ) );
    }
}

Lumen::TExpected<VkAttachmentStoreOp, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanAttachmentStoreOp ( RHI::ERHIRenderingStoreOp InOp ) noexcept
{
    switch ( InOp )
    {
    case RHI::ERHIRenderingStoreOp::Store:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case RHI::ERHIRenderingStoreOp::DontCare:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidCommand( "Invalid ERHIRenderingStoreOp value." ) );
    }
}

Lumen::TExpected<VkQueryType, Lumen::RHI::FRHIError> Lumen::RHIVulkan::ToVulkanQueryType ( RHI::ERHIQueryType InType ) noexcept
{
    switch ( InType )
    {
    case RHI::ERHIQueryType::Occlusion:
        return VK_QUERY_TYPE_OCCLUSION;
    case RHI::ERHIQueryType::Timestamp:
        return VK_QUERY_TYPE_TIMESTAMP;
    case RHI::ERHIQueryType::PipelineStatistics:
        return VK_QUERY_TYPE_PIPELINE_STATISTICS;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidQuery( "Invalid ERHIQueryType value." ) );
    }
}

Lumen::TExpected<VkPresentModeKHR, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanPresentMode ( RHI::ERHIPresentMode InMode ) noexcept
{
    switch ( InMode )
    {
    case RHI::ERHIPresentMode::Fifo:
        return VK_PRESENT_MODE_FIFO_KHR;
    case RHI::ERHIPresentMode::Immediate:
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    case RHI::ERHIPresentMode::Mailbox:
        return VK_PRESENT_MODE_MAILBOX_KHR;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidPresentation( "Invalid ERHIPresentMode value." ) );
    }
}

Lumen::TOptional<Lumen::RHI::ERHIPresentMode> Lumen::RHIVulkan::ToRHIPresentMode ( VkPresentModeKHR InMode ) noexcept
{
    switch ( InMode )
    {
    case VK_PRESENT_MODE_FIFO_KHR:
        return TOptional{ RHI::ERHIPresentMode::Fifo };
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        return TOptional{ RHI::ERHIPresentMode::Immediate };
    case VK_PRESENT_MODE_MAILBOX_KHR:
        return TOptional{ RHI::ERHIPresentMode::Mailbox };
    default:
        [[unlikely]] return {};
    }
}

Lumen::TExpected<VkColorSpaceKHR, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanColorSpace ( RHI::ERHIColorSpace InColorSpace ) noexcept
{
    switch ( InColorSpace )
    {
    case RHI::ERHIColorSpace::SrgbNonLinear:
        return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    case RHI::ERHIColorSpace::DisplayP3NonLinear:
        return VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT;
    case RHI::ERHIColorSpace::Hdr10St2084:
        return VK_COLOR_SPACE_HDR10_ST2084_EXT;
    case RHI::ERHIColorSpace::ExtendedSrgbLinear:
        return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
    default:
        [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidPresentation( "Invalid ERHIColorSpace value." ) );
    }
}

Lumen::TOptional<Lumen::RHI::ERHIColorSpace> Lumen::RHIVulkan::ToRHIColorSpace ( VkColorSpaceKHR InColorSpace ) noexcept
{
    switch ( InColorSpace )
    {
    case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
        return TOptional{ RHI::ERHIColorSpace::SrgbNonLinear };
    case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
        return TOptional{ RHI::ERHIColorSpace::DisplayP3NonLinear };
    case VK_COLOR_SPACE_HDR10_ST2084_EXT:
        return TOptional{ RHI::ERHIColorSpace::Hdr10St2084 };
    case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
        return TOptional{ RHI::ERHIColorSpace::ExtendedSrgbLinear };
    default:
        [[unlikely]] return {};
    }
}

Lumen::Bool Lumen::RHIVulkan::IsVulkanQueueFamilyCompatible ( VkQueueFlags InQueueFlags, RHI::ERHIQueueType InQueueType ) noexcept
{
    switch ( InQueueType )
    {
    case RHI::ERHIQueueType::Graphics:
        return ( InQueueFlags & VK_QUEUE_GRAPHICS_BIT ) != 0U;
    case RHI::ERHIQueueType::Compute:
        return ( InQueueFlags & VK_QUEUE_COMPUTE_BIT ) != 0U;
    case RHI::ERHIQueueType::Copy:
        /** NOTE: In Vulkan, graphics & compute queues implicitly support transfer operations. */
        return ( InQueueFlags & ( VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) ) != 0U;
    default:
        [[unlikely]] return false;
    }
}
