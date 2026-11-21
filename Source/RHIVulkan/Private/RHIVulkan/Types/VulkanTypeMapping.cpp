/**
 * @file VulkanTypeMapping.cpp
 * @brief Implements portable RHI-to-Vulkan enum, format, usage, and state mappings.
 */

#include <RHI/Resources/RHIResourceUsage.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIResourceState.hpp>

#include <RHIVulkan/Types/VulkanTypeMapping.hpp>

#include <RHI/Types/RHIFormatInfo.hpp>

#include <Core/CoreTypes.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * Private generators
     */

#define LUMEN_VK_ENUM_MAP_CASE( RhiName, VkName )                                                                                \
    case RhiName:                                                                                                                \
        return VkName;

#define LUMEN_VK_DEFINE_ENUM_MAP( FnName, RhiEnum, VkType, List, MakeError )                                                     \
    TExpected<VkType, RHI::FRHIError> FnName( RhiEnum InValue ) noexcept                                                              \
    {                                                                                                                            \
        switch ( InValue )                                                                                                       \
        {                                                                                                                        \
            List( LUMEN_VK_ENUM_MAP_CASE ) default : [[unlikely]] return TUnexpected( MakeError );                               \
        }                                                                                                                        \
    }

#define LUMEN_VK_ENUM_UNMAP_CASE( RhiName, VkName )                                                                              \
    case VkName:                                                                                                                 \
        return TOptional{ RhiName };

#define LUMEN_VK_DEFINE_ENUM_UNMAP( FnName, RhiEnum, VkType, List )                                                              \
    TOptional<RhiEnum> FnName( VkType InValue ) noexcept                                                                         \
    {                                                                                                                            \
        switch ( InValue )                                                                                                       \
        {                                                                                                                        \
            List( LUMEN_VK_ENUM_UNMAP_CASE ) default : [[unlikely]] return {};                                                   \
        }                                                                                                                        \
    }

/** @brief OR-chain row; expects local `InValue` and `Flags` in the enclosing function. */
#define LUMEN_VK_FLAG_OR( RhiName, VkName )                                                                                      \
    if ( HasAnyFlags( InValue, RhiName ) )                                                                                       \
    {                                                                                                                            \
        Flags |= VkName;                                                                                                         \
    }

    /**
     * Formats (bidirectional)
     */

#define LUMEN_VK_FORMAT_LIST( X )                                                                                                \
    X( RHI::ERHIFormat::R8Unorm, VK_FORMAT_R8_UNORM )                                                                            \
    X( RHI::ERHIFormat::R8Snorm, VK_FORMAT_R8_SNORM )                                                                            \
    X( RHI::ERHIFormat::R8Uint, VK_FORMAT_R8_UINT )                                                                              \
    X( RHI::ERHIFormat::R8Sint, VK_FORMAT_R8_SINT )                                                                              \
    X( RHI::ERHIFormat::RG8Unorm, VK_FORMAT_R8G8_UNORM )                                                                         \
    X( RHI::ERHIFormat::RG8Snorm, VK_FORMAT_R8G8_SNORM )                                                                         \
    X( RHI::ERHIFormat::RG8Uint, VK_FORMAT_R8G8_UINT )                                                                           \
    X( RHI::ERHIFormat::RG8Sint, VK_FORMAT_R8G8_SINT )                                                                           \
    X( RHI::ERHIFormat::RGBA8Unorm, VK_FORMAT_R8G8B8A8_UNORM )                                                                   \
    X( RHI::ERHIFormat::RGBA8Snorm, VK_FORMAT_R8G8B8A8_SNORM )                                                                   \
    X( RHI::ERHIFormat::RGBA8Uint, VK_FORMAT_R8G8B8A8_UINT )                                                                     \
    X( RHI::ERHIFormat::RGBA8Sint, VK_FORMAT_R8G8B8A8_SINT )                                                                     \
    X( RHI::ERHIFormat::RGBA8Srgb, VK_FORMAT_R8G8B8A8_SRGB )                                                                     \
    X( RHI::ERHIFormat::BGRA8Unorm, VK_FORMAT_B8G8R8A8_UNORM )                                                                   \
    X( RHI::ERHIFormat::BGRA8Srgb, VK_FORMAT_B8G8R8A8_SRGB )                                                                     \
    X( RHI::ERHIFormat::R16Unorm, VK_FORMAT_R16_UNORM )                                                                          \
    X( RHI::ERHIFormat::R16Snorm, VK_FORMAT_R16_SNORM )                                                                          \
    X( RHI::ERHIFormat::R16Uint, VK_FORMAT_R16_UINT )                                                                            \
    X( RHI::ERHIFormat::R16Sint, VK_FORMAT_R16_SINT )                                                                            \
    X( RHI::ERHIFormat::R16Float, VK_FORMAT_R16_SFLOAT )                                                                         \
    X( RHI::ERHIFormat::RG16Unorm, VK_FORMAT_R16G16_UNORM )                                                                      \
    X( RHI::ERHIFormat::RG16Snorm, VK_FORMAT_R16G16_SNORM )                                                                      \
    X( RHI::ERHIFormat::RG16Uint, VK_FORMAT_R16G16_UINT )                                                                        \
    X( RHI::ERHIFormat::RG16Sint, VK_FORMAT_R16G16_SINT )                                                                        \
    X( RHI::ERHIFormat::RG16Float, VK_FORMAT_R16G16_SFLOAT )                                                                     \
    X( RHI::ERHIFormat::RGBA16Unorm, VK_FORMAT_R16G16B16A16_UNORM )                                                              \
    X( RHI::ERHIFormat::RGBA16Snorm, VK_FORMAT_R16G16B16A16_SNORM )                                                              \
    X( RHI::ERHIFormat::RGBA16Uint, VK_FORMAT_R16G16B16A16_UINT )                                                                \
    X( RHI::ERHIFormat::RGBA16Sint, VK_FORMAT_R16G16B16A16_SINT )                                                                \
    X( RHI::ERHIFormat::RGBA16Float, VK_FORMAT_R16G16B16A16_SFLOAT )                                                             \
    X( RHI::ERHIFormat::R32Uint, VK_FORMAT_R32_UINT )                                                                            \
    X( RHI::ERHIFormat::R32Sint, VK_FORMAT_R32_SINT )                                                                            \
    X( RHI::ERHIFormat::R32Float, VK_FORMAT_R32_SFLOAT )                                                                         \
    X( RHI::ERHIFormat::RG32Uint, VK_FORMAT_R32G32_UINT )                                                                        \
    X( RHI::ERHIFormat::RG32Sint, VK_FORMAT_R32G32_SINT )                                                                        \
    X( RHI::ERHIFormat::RG32Float, VK_FORMAT_R32G32_SFLOAT )                                                                     \
    X( RHI::ERHIFormat::RGB32Uint, VK_FORMAT_R32G32B32_UINT )                                                                    \
    X( RHI::ERHIFormat::RGB32Sint, VK_FORMAT_R32G32B32_SINT )                                                                    \
    X( RHI::ERHIFormat::RGB32Float, VK_FORMAT_R32G32B32_SFLOAT )                                                                 \
    X( RHI::ERHIFormat::RGBA32Uint, VK_FORMAT_R32G32B32A32_UINT )                                                                \
    X( RHI::ERHIFormat::RGBA32Sint, VK_FORMAT_R32G32B32A32_SINT )                                                                \
    X( RHI::ERHIFormat::RGBA32Float, VK_FORMAT_R32G32B32A32_SFLOAT )                                                             \
    X( RHI::ERHIFormat::D16Unorm, VK_FORMAT_D16_UNORM )                                                                          \
    X( RHI::ERHIFormat::D24UnormS8Uint, VK_FORMAT_D24_UNORM_S8_UINT )                                                            \
    X( RHI::ERHIFormat::D32Float, VK_FORMAT_D32_SFLOAT )                                                                         \
    X( RHI::ERHIFormat::D32FloatS8Uint, VK_FORMAT_D32_SFLOAT_S8_UINT )                                                           \
    X( RHI::ERHIFormat::BC1RGBAUnorm, VK_FORMAT_BC1_RGBA_UNORM_BLOCK )                                                           \
    X( RHI::ERHIFormat::BC1RGBASrgb, VK_FORMAT_BC1_RGBA_SRGB_BLOCK )                                                             \
    X( RHI::ERHIFormat::BC3RGBAUnorm, VK_FORMAT_BC3_UNORM_BLOCK )                                                                \
    X( RHI::ERHIFormat::BC3RGBASrgb, VK_FORMAT_BC3_SRGB_BLOCK )                                                                  \
    X( RHI::ERHIFormat::BC7RGBAUnorm, VK_FORMAT_BC7_UNORM_BLOCK )                                                                \
    X( RHI::ERHIFormat::BC7RGBASrgb, VK_FORMAT_BC7_SRGB_BLOCK )

    namespace
    {

        [[nodiscard]] TExpected<VkFormat, RHI::FRHIError> MapFormatInternal ( RHI::ERHIFormat InFormat, Bool bAllowUnknown ) noexcept
        {
            if ( InFormat == RHI::ERHIFormat::Unknown )
            {
                if ( bAllowUnknown )
                {
                    return VK_FORMAT_UNDEFINED;
                }
                return TUnexpected( RHI::MakeRHIInvalidResource( "RHI::ERHIFormat::Unknown is not a valid concrete format." ) );
            }

            switch ( InFormat )
            {
                LUMEN_VK_FORMAT_LIST( LUMEN_VK_ENUM_MAP_CASE )
            default:
                return TUnexpected( RHI::MakeRHIInvalidResource( "Unrecognized RHI::ERHIFormat value." ) );
            }
        }

    } // namespace

    TExpected<VkFormat, RHI::FRHIError> ToVulkanFormat ( RHI::ERHIFormat InFormat ) noexcept
    {
        return MapFormatInternal( InFormat, false );
    }

    TExpected<VkFormat, RHI::FRHIError> ToVulkanFormatOrUndefined ( RHI::ERHIFormat InFormat ) noexcept
    {
        return MapFormatInternal( InFormat, true );
    }

    RHI::ERHIFormat ToRHIFormat ( VkFormat InFormat ) noexcept
    {
        switch ( InFormat )
        {
#define LUMEN_VK_FORMAT_UNMAP_CASE( RhiName, VkName )                                                                            \
    case VkName:                                                                                                                 \
        return RhiName;
            LUMEN_VK_FORMAT_LIST( LUMEN_VK_FORMAT_UNMAP_CASE )
#undef LUMEN_VK_FORMAT_UNMAP_CASE
        default:
            return RHI::ERHIFormat::Unknown;
        }
    }

    /**
     * Image aspects
     */

#define LUMEN_VK_IMAGE_ASPECT_LIST( X )                                                                                          \
    X( RHI::ERHIFormatAspect::Color, VK_IMAGE_ASPECT_COLOR_BIT )                                                                 \
    X( RHI::ERHIFormatAspect::Depth, VK_IMAGE_ASPECT_DEPTH_BIT )                                                                 \
    X( RHI::ERHIFormatAspect::Stencil, VK_IMAGE_ASPECT_STENCIL_BIT )

    TExpected<VkImageAspectFlags, RHI::FRHIError> ToVulkanImageAspect ( RHI::ERHIFormatAspect InAspect ) noexcept
    {
        constexpr UInt8 ValidMask =
            static_cast<UInt8>( RHI::ERHIFormatAspect::Color | RHI::ERHIFormatAspect::Depth | RHI::ERHIFormatAspect::Stencil );
        const UInt8 Raw = static_cast<UInt8>( InAspect );
        if ( Raw == 0U or ( Raw & ~ValidMask ) != 0U )
        {
            return TUnexpected( RHI::MakeRHIInvalidResource( "Invalid RHI::ERHIFormatAspect bitmask." ) );
        }

        const RHI::ERHIFormatAspect InValue = InAspect;
        VkImageAspectFlags Flags            = 0U;
        LUMEN_VK_IMAGE_ASPECT_LIST( LUMEN_VK_FLAG_OR )
        return Flags;
    }

    TExpected<VkImageAspectFlags, RHI::FRHIError> GetVulkanFullImageAspect ( RHI::ERHIFormat InFormat ) noexcept
    {
        const TOptional<RHI::FRHIFormatInfo> Info = TryGetRHIFormatInfo( InFormat );

        if ( not Info or Info->Aspects == RHI::ERHIFormatAspect::None ) [[unlikely]]
        {
            return TUnexpected( RHI::MakeRHIInvalidResource( "Cannot derive Vulkan image aspects for Unknown or invalid format." ) );
        }
        return ToVulkanImageAspect( Info->Aspects );
    }

    /**
     * Resource usage
     */

#define LUMEN_VK_BUFFER_USAGE_LIST( X )                                                                                          \
    X( RHI::ERHIResourceUsage::CopySource, VK_BUFFER_USAGE_TRANSFER_SRC_BIT )                                                    \
    X( ( RHI::ERHIResourceUsage::CopyDestination | RHI::ERHIResourceUsage::QueryResolve ), VK_BUFFER_USAGE_TRANSFER_DST_BIT )    \
    X( RHI::ERHIResourceUsage::VertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT )                                                 \
    X( RHI::ERHIResourceUsage::IndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT )                                                   \
    X( RHI::ERHIResourceUsage::ConstantBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT )                                              \
    X( ( RHI::ERHIResourceUsage::ShaderResource | RHI::ERHIResourceUsage::UnorderedAccess ),                                     \
       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT )                                                                                      \
    X( RHI::ERHIResourceUsage::IndirectArgument, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT )

#define LUMEN_VK_IMAGE_USAGE_LIST( X )                                                                                           \
    X( RHI::ERHIResourceUsage::CopySource, VK_IMAGE_USAGE_TRANSFER_SRC_BIT )                                                     \
    X( RHI::ERHIResourceUsage::CopyDestination, VK_IMAGE_USAGE_TRANSFER_DST_BIT )                                                \
    X( RHI::ERHIResourceUsage::ShaderResource, VK_IMAGE_USAGE_SAMPLED_BIT )                                                      \
    X( RHI::ERHIResourceUsage::UnorderedAccess, VK_IMAGE_USAGE_STORAGE_BIT )                                                     \
    X( RHI::ERHIResourceUsage::RenderTarget, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )                                               \
    X( RHI::ERHIResourceUsage::DepthStencil, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )

    TExpected<VkBufferUsageFlags, RHI::FRHIError> ToVulkanBufferUsage ( RHI::ERHIResourceUsage InUsage ) noexcept
    {
        if ( not IsValidRHIResourceUsage( InUsage ) or InUsage == RHI::ERHIResourceUsage::None ) [[unlikely]]
        {
            return TUnexpected( RHI::MakeRHIInvalidResource( "Buffer usage flags must be valid and nonzero." ) );
        }

        if ( HasAnyFlags( InUsage, RHI::ERHIResourceUsage::RenderTarget | RHI::ERHIResourceUsage::DepthStencil ) ) [[unlikely]]
        {
            return TUnexpected( RHI::MakeRHIInvalidResource( "RenderTarget and DepthStencil usages are invalid for buffers." ) );
        }

        const RHI::ERHIResourceUsage InValue = InUsage;
        VkBufferUsageFlags Flags             = 0U;
        LUMEN_VK_BUFFER_USAGE_LIST( LUMEN_VK_FLAG_OR )
        return Flags;
    }

    TExpected<VkImageUsageFlags, RHI::FRHIError> ToVulkanImageUsage ( RHI::ERHIResourceUsage InUsage ) noexcept
    {
        if ( not IsValidRHIResourceUsage( InUsage ) or InUsage == RHI::ERHIResourceUsage::None ) [[unlikely]]
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

        const RHI::ERHIResourceUsage InValue = InUsage;
        VkImageUsageFlags Flags              = 0U;
        LUMEN_VK_IMAGE_USAGE_LIST( LUMEN_VK_FLAG_OR )
        return Flags;
    }

    /**
     * Buffer barrier states
     */

#define LUMEN_VK_BUFFER_STATE_LIST( X )                                                                                          \
    X( RHI::ERHIResourceState::Common, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,                                                     \
       ( VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT ) )                                                          \
    X( RHI::ERHIResourceState::VertexBuffer, VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,                                     \
       VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT )                                                                                   \
    X( RHI::ERHIResourceState::IndexBuffer, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_ACCESS_2_INDEX_READ_BIT )                    \
    X( RHI::ERHIResourceState::ConstantBuffer,                                                                                   \
       ( VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT ), VK_ACCESS_2_UNIFORM_READ_BIT )         \
    X( RHI::ERHIResourceState::ShaderResource,                                                                                   \
       ( VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT ), VK_ACCESS_2_SHADER_READ_BIT )          \
    X( RHI::ERHIResourceState::UnorderedAccess,                                                                                  \
       ( VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT ),                                        \
       ( VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT ) )                                          \
    X( RHI::ERHIResourceState::CopySource, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT )                         \
    X( RHI::ERHIResourceState::CopyDestination, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT )                   \
    X( RHI::ERHIResourceState::ResolveSource, VK_PIPELINE_STAGE_2_RESOLVE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT )                   \
    X( RHI::ERHIResourceState::ResolveDestination, VK_PIPELINE_STAGE_2_RESOLVE_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT )             \
    X( RHI::ERHIResourceState::IndirectArgument, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT )

#define LUMEN_VK_BUFFER_STATE_ROW( RhiState, Stages, Access )                                                                    \
    if ( HasAnyFlags( InState, RhiState ) )                                                                                      \
    {                                                                                                                            \
        Result.StageMask |= Stages;                                                                                              \
        Result.AccessMask |= Access;                                                                                             \
    }

    TExpected<FVulkanResourceBarrierState, RHI::FRHIError> ToVulkanBufferState ( RHI::ERHIResourceState InState ) noexcept
    {
        if ( not RHI::IsValidRHIResourceState( InState ) ) [[unlikely]]
        {
            return TUnexpected( RHI::MakeRHIInvalidResource( "Unknown RHI::ERHIResourceState bits." ) );
        }

        constexpr RHI::ERHIResourceState ImageOnlyStates = RHI::ERHIResourceState::RenderTarget |
                                                           RHI::ERHIResourceState::DepthWrite |
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

        LUMEN_VK_BUFFER_STATE_LIST( LUMEN_VK_BUFFER_STATE_ROW )
        return Result;
    }

    /**
     * Image barrier states (non-tabular: layouts depend on depth/stencil and exclusive combinations)
     */

    TExpected<FVulkanResourceBarrierState, RHI::FRHIError> ToVulkanImageState ( RHI::ERHIResourceState InState,
                                                                           Bool bIsDepthStencil ) noexcept
    {
        if ( not IsValidRHIResourceState( InState ) ) [[unlikely]]
        {
            return TUnexpected( RHI::MakeRHIInvalidResource( "Unknown RHI::ERHIResourceState bits." ) );
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
                return TUnexpected( RHI::MakeRHIInvalidResource( "RenderTarget state is incompatible with depth/stencil textures." ) );
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

        constexpr RHI::ERHIResourceState TransferSrcMask =
            RHI::ERHIResourceState::CopySource | RHI::ERHIResourceState::ResolveSource;
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
            RHI::MakeRHIInvalidResource( "Combined RHI::ERHIResourceState flags require incompatible Vulkan image layouts." ) );
    }

    /**
     * Sample count
     */

#define LUMEN_VK_SAMPLE_COUNT_LIST( X )                                                                                          \
    X( RHI::ERHISampleCount::Count1, VK_SAMPLE_COUNT_1_BIT )                                                                     \
    X( RHI::ERHISampleCount::Count2, VK_SAMPLE_COUNT_2_BIT )                                                                     \
    X( RHI::ERHISampleCount::Count4, VK_SAMPLE_COUNT_4_BIT )                                                                     \
    X( RHI::ERHISampleCount::Count8, VK_SAMPLE_COUNT_8_BIT )                                                                     \
    X( RHI::ERHISampleCount::Count16, VK_SAMPLE_COUNT_16_BIT )                                                                   \
    X( RHI::ERHISampleCount::Count32, VK_SAMPLE_COUNT_32_BIT )                                                                   \
    X( RHI::ERHISampleCount::Count64, VK_SAMPLE_COUNT_64_BIT )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanSampleCount,
                              RHI::ERHISampleCount,
                              VkSampleCountFlagBits,
                              LUMEN_VK_SAMPLE_COUNT_LIST,
                              RHI::MakeRHIInvalidPipeline( "Invalid RHI::ERHISampleCount value." ) )

    /**
     * Shader stages (shared list: single-stage switch + OR-chain)
     */

#define LUMEN_VK_SHADER_STAGE_LIST( X )                                                                                          \
    X( RHI::ERHIShaderStage::Vertex, VK_SHADER_STAGE_VERTEX_BIT )                                                                \
    X( RHI::ERHIShaderStage::Pixel, VK_SHADER_STAGE_FRAGMENT_BIT )                                                               \
    X( RHI::ERHIShaderStage::Compute, VK_SHADER_STAGE_COMPUTE_BIT )                                                              \
    X( RHI::ERHIShaderStage::Geometry, VK_SHADER_STAGE_GEOMETRY_BIT )                                                            \
    X( RHI::ERHIShaderStage::Hull, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT )                                                    \
    X( RHI::ERHIShaderStage::Domain, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT )                                               \
    X( RHI::ERHIShaderStage::Amplification, VK_SHADER_STAGE_TASK_BIT_EXT )                                                       \
    X( RHI::ERHIShaderStage::Mesh, VK_SHADER_STAGE_MESH_BIT_EXT )

    TExpected<VkShaderStageFlags, RHI::FRHIError> ToVulkanShaderStageFlags ( RHI::ERHIShaderStage InStages ) noexcept
    {
        const UInt32 Raw     = static_cast<UInt32>( InStages );
        const UInt32 AllMask = static_cast<UInt32>( RHI::ERHIShaderStage::All );
        if ( Raw == 0U or ( Raw & ~AllMask ) != 0U )
        {
            return TUnexpected( RHI::MakeRHIInvalidShader( "Invalid or empty RHI::ERHIShaderStage mask." ) );
        }

        if ( InStages == RHI::ERHIShaderStage::All )
        {
            return VK_SHADER_STAGE_ALL;
        }
        if ( InStages == RHI::ERHIShaderStage::AllGraphics )
        {
            return VK_SHADER_STAGE_ALL_GRAPHICS;
        }

        const RHI::ERHIShaderStage InValue = InStages;
        VkShaderStageFlags Flags           = 0U;
        LUMEN_VK_SHADER_STAGE_LIST( LUMEN_VK_FLAG_OR )
        return Flags;
    }

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanSingleShaderStage,
                              RHI::ERHIShaderStage,
                              VkShaderStageFlagBits,
                              LUMEN_VK_SHADER_STAGE_LIST,
                              RHI::MakeRHIInvalidShader( "RHI::ERHIShaderStage must specify a single valid stage." ) )

    /**
     * Descriptor types
     */

#define LUMEN_VK_DESCRIPTOR_TYPE_LIST( X )                                                                                       \
    X( RHI::ERHIBindingType::ConstantBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER )                                                 \
    X( RHI::ERHIBindingType::ReadOnlyBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER )                                                 \
    X( RHI::ERHIBindingType::ReadWriteBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER )                                                \
    X( RHI::ERHIBindingType::Texture, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE )                                                         \
    X( RHI::ERHIBindingType::StorageTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE )                                                  \
    X( RHI::ERHIBindingType::Sampler, VK_DESCRIPTOR_TYPE_SAMPLER )                                                               \
    X( RHI::ERHIBindingType::CombinedTextureSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanDescriptorType,
                              RHI::ERHIBindingType,
                              VkDescriptorType,
                              LUMEN_VK_DESCRIPTOR_TYPE_LIST,
                              RHI::MakeRHIInvalidBinding( "Invalid RHI::ERHIBindingType value." ) )

    /**
     * Index format
     */

#define LUMEN_VK_INDEX_TYPE_LIST( X )                                                                                            \
    X( RHI::ERHIIndexFormat::Index16, VK_INDEX_TYPE_UINT16 )                                                                     \
    X( RHI::ERHIIndexFormat::Index32, VK_INDEX_TYPE_UINT32 )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanIndexType,
                              RHI::ERHIIndexFormat,
                              VkIndexType,
                              LUMEN_VK_INDEX_TYPE_LIST,
                              RHI::MakeRHIInvalidCommand( "Invalid RHI::ERHIIndexFormat value." ) )

    /**
     * Texture view type
     */

#define LUMEN_VK_IMAGE_VIEW_TYPE_LIST( X )                                                                                       \
    X( RHI::ERHITextureViewType::Texture1D, VK_IMAGE_VIEW_TYPE_1D )                                                              \
    X( RHI::ERHITextureViewType::Texture2D, VK_IMAGE_VIEW_TYPE_2D )                                                              \
    X( RHI::ERHITextureViewType::Texture3D, VK_IMAGE_VIEW_TYPE_3D )                                                              \
    X( RHI::ERHITextureViewType::TextureCube, VK_IMAGE_VIEW_TYPE_CUBE )                                                          \
    X( RHI::ERHITextureViewType::Texture2DArray, VK_IMAGE_VIEW_TYPE_2D_ARRAY )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanImageViewType,
                              RHI::ERHITextureViewType,
                              VkImageViewType,
                              LUMEN_VK_IMAGE_VIEW_TYPE_LIST,
                              RHI::MakeRHIInvalidResource( "Invalid RHI::ERHITextureViewType value." ) )

    /**
     * Primitive topology
     */

#define LUMEN_VK_PRIMITIVE_TOPOLOGY_LIST( X )                                                                                    \
    X( RHI::ERHIPrimitiveTopology::PointList, VK_PRIMITIVE_TOPOLOGY_POINT_LIST )                                                 \
    X( RHI::ERHIPrimitiveTopology::LineList, VK_PRIMITIVE_TOPOLOGY_LINE_LIST )                                                   \
    X( RHI::ERHIPrimitiveTopology::LineStrip, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP )                                                 \
    X( RHI::ERHIPrimitiveTopology::TriangleList, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST )                                           \
    X( RHI::ERHIPrimitiveTopology::TriangleStrip, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP )                                         \
    X( RHI::ERHIPrimitiveTopology::PatchList, VK_PRIMITIVE_TOPOLOGY_PATCH_LIST )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanPrimitiveTopology,
                              RHI::ERHIPrimitiveTopology,
                              VkPrimitiveTopology,
                              LUMEN_VK_PRIMITIVE_TOPOLOGY_LIST,
                              RHI::MakeRHIInvalidPipeline( "Invalid RHI::ERHIPrimitiveTopology value." ) )

    /**
     * Rasterizer state
     */

#define LUMEN_VK_POLYGON_MODE_LIST( X )                                                                                          \
    X( RHI::ERHIFillMode::Solid, VK_POLYGON_MODE_FILL )                                                                          \
    X( RHI::ERHIFillMode::Wireframe, VK_POLYGON_MODE_LINE )                                                                      \
    X( RHI::ERHIFillMode::Point, VK_POLYGON_MODE_POINT )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanPolygonMode,
                              RHI::ERHIFillMode,
                              VkPolygonMode,
                              LUMEN_VK_POLYGON_MODE_LIST,
                              RHI::MakeRHIInvalidPipeline( "Invalid RHI::ERHIFillMode value." ) )

#define LUMEN_VK_CULL_MODE_LIST( X )                                                                                             \
    X( RHI::ERHICullMode::None, VK_CULL_MODE_NONE )                                                                              \
    X( RHI::ERHICullMode::Front, VK_CULL_MODE_FRONT_BIT )                                                                        \
    X( RHI::ERHICullMode::Back, VK_CULL_MODE_BACK_BIT )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanCullMode,
                              RHI::ERHICullMode,
                              VkCullModeFlags,
                              LUMEN_VK_CULL_MODE_LIST,
                              RHI::MakeRHIInvalidPipeline( "Invalid RHI::ERHICullMode value." ) )

#define LUMEN_VK_FRONT_FACE_LIST( X )                                                                                            \
    X( RHI::ERHIFrontFace::CounterClockwise, VK_FRONT_FACE_COUNTER_CLOCKWISE )                                                   \
    X( RHI::ERHIFrontFace::Clockwise, VK_FRONT_FACE_CLOCKWISE )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanFrontFace,
                              RHI::ERHIFrontFace,
                              VkFrontFace,
                              LUMEN_VK_FRONT_FACE_LIST,
                              RHI::MakeRHIInvalidPipeline( "Invalid RHI::ERHIFrontFace value." ) )

    /**
     * Blend state
     */

#define LUMEN_VK_BLEND_FACTOR_LIST( X )                                                                                          \
    X( RHI::ERHIBlendFactor::Zero, VK_BLEND_FACTOR_ZERO )                                                                        \
    X( RHI::ERHIBlendFactor::One, VK_BLEND_FACTOR_ONE )                                                                          \
    X( RHI::ERHIBlendFactor::SourceAlpha, VK_BLEND_FACTOR_SRC_ALPHA )                                                            \
    X( RHI::ERHIBlendFactor::InverseSourceAlpha, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA )                                           \
    X( RHI::ERHIBlendFactor::DestinationColor, VK_BLEND_FACTOR_DST_COLOR )                                                       \
    X( RHI::ERHIBlendFactor::InverseDestinationColor, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanBlendFactor,
                              RHI::ERHIBlendFactor,
                              VkBlendFactor,
                              LUMEN_VK_BLEND_FACTOR_LIST,
                              RHI::MakeRHIInvalidPipeline( "Invalid RHI::ERHIBlendFactor value." ) )

#define LUMEN_VK_BLEND_OP_LIST( X )                                                                                              \
    X( RHI::ERHIBlendOp::Add, VK_BLEND_OP_ADD )                                                                                  \
    X( RHI::ERHIBlendOp::Subtract, VK_BLEND_OP_SUBTRACT )                                                                        \
    X( RHI::ERHIBlendOp::ReverseSubtract, VK_BLEND_OP_REVERSE_SUBTRACT )                                                         \
    X( RHI::ERHIBlendOp::Min, VK_BLEND_OP_MIN )                                                                                  \
    X( RHI::ERHIBlendOp::Max, VK_BLEND_OP_MAX )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanBlendOp,
                              RHI::ERHIBlendOp,
                              VkBlendOp,
                              LUMEN_VK_BLEND_OP_LIST,
                              RHI::MakeRHIInvalidPipeline( "Invalid RHI::ERHIBlendOp value." ) )

    /**
     * Depth-stencil state
     */

#define LUMEN_VK_STENCIL_OP_LIST( X )                                                                                            \
    X( RHI::ERHIStencilOp::Keep, VK_STENCIL_OP_KEEP )                                                                            \
    X( RHI::ERHIStencilOp::Zero, VK_STENCIL_OP_ZERO )                                                                            \
    X( RHI::ERHIStencilOp::Replace, VK_STENCIL_OP_REPLACE )                                                                      \
    X( RHI::ERHIStencilOp::IncrementClamp, VK_STENCIL_OP_INCREMENT_AND_CLAMP )                                                   \
    X( RHI::ERHIStencilOp::DecrementClamp, VK_STENCIL_OP_DECREMENT_AND_CLAMP )                                                   \
    X( RHI::ERHIStencilOp::Invert, VK_STENCIL_OP_INVERT )                                                                        \
    X( RHI::ERHIStencilOp::IncrementWrap, VK_STENCIL_OP_INCREMENT_AND_WRAP )                                                     \
    X( RHI::ERHIStencilOp::DecrementWrap, VK_STENCIL_OP_DECREMENT_AND_WRAP )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanStencilOp,
                              RHI::ERHIStencilOp,
                              VkStencilOp,
                              LUMEN_VK_STENCIL_OP_LIST,
                              RHI::MakeRHIInvalidPipeline( "Invalid RHI::ERHIStencilOp value." ) )

#define LUMEN_VK_COMPARE_OP_LIST( X )                                                                                            \
    X( RHI::ERHICompareFunction::Never, VK_COMPARE_OP_NEVER )                                                                    \
    X( RHI::ERHICompareFunction::Less, VK_COMPARE_OP_LESS )                                                                      \
    X( RHI::ERHICompareFunction::Equal, VK_COMPARE_OP_EQUAL )                                                                    \
    X( RHI::ERHICompareFunction::LessEqual, VK_COMPARE_OP_LESS_OR_EQUAL )                                                        \
    X( RHI::ERHICompareFunction::Greater, VK_COMPARE_OP_GREATER )                                                                \
    X( RHI::ERHICompareFunction::NotEqual, VK_COMPARE_OP_NOT_EQUAL )                                                             \
    X( RHI::ERHICompareFunction::GreaterEqual, VK_COMPARE_OP_GREATER_OR_EQUAL )                                                  \
    X( RHI::ERHICompareFunction::Always, VK_COMPARE_OP_ALWAYS )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanCompareOp,
                              RHI::ERHICompareFunction,
                              VkCompareOp,
                              LUMEN_VK_COMPARE_OP_LIST,
                              RHI::MakeRHIInvalidPipeline( "Invalid RHI::ERHICompareFunction value." ) )

    /**
     * Sampler state (shared 3-column list: filter + mipmap mode)
     */

#define LUMEN_VK_SAMPLER_FILTER_LIST( X )                                                                                        \
    X( RHI::ERHISamplerFilter::Nearest, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST )                                      \
    X( RHI::ERHISamplerFilter::Linear, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR )

    TExpected<VkFilter, RHI::FRHIError> ToVulkanFilter ( RHI::ERHISamplerFilter InFilter ) noexcept
    {
        switch ( InFilter )
        {
#define LUMEN_VK_FILTER_CASE( RhiName, VkFilter, VkMipMode )                                                                     \
    case RhiName:                                                                                                                \
        return VkFilter;
            LUMEN_VK_SAMPLER_FILTER_LIST( LUMEN_VK_FILTER_CASE )
#undef LUMEN_VK_FILTER_CASE
        default:
            [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidResource( "Invalid RHI::ERHISamplerFilter value." ) );
        }
    }

    TExpected<VkSamplerMipmapMode, RHI::FRHIError> ToVulkanMipmapMode ( RHI::ERHISamplerFilter InFilter ) noexcept
    {
        switch ( InFilter )
        {
#define LUMEN_VK_MIPMAP_CASE( RhiName, VkFilter, VkMipMode )                                                                     \
    case RhiName:                                                                                                                \
        return VkMipMode;
            LUMEN_VK_SAMPLER_FILTER_LIST( LUMEN_VK_MIPMAP_CASE )
#undef LUMEN_VK_MIPMAP_CASE
        default:
            [[unlikely]] return TUnexpected( RHI::MakeRHIInvalidResource( "Invalid RHI::ERHISamplerFilter value." ) );
        }
    }

#define LUMEN_VK_ADDRESS_MODE_LIST( X )                                                                                          \
    X( RHI::ERHISamplerAddressMode::Repeat, VK_SAMPLER_ADDRESS_MODE_REPEAT )                                                     \
    X( RHI::ERHISamplerAddressMode::MirroredRepeat, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT )                                    \
    X( RHI::ERHISamplerAddressMode::ClampToEdge, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE )                                         \
    X( RHI::ERHISamplerAddressMode::ClampToBorder, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanAddressMode,
                              RHI::ERHISamplerAddressMode,
                              VkSamplerAddressMode,
                              LUMEN_VK_ADDRESS_MODE_LIST,
                              RHI::MakeRHIInvalidResource( "Invalid RHI::ERHISamplerAddressMode value." ) )

    /**
     * Rendering attachment ops
     */

#define LUMEN_VK_ATTACHMENT_LOAD_OP_LIST( X )                                                                                    \
    X( RHI::ERHIRenderingLoadOp::Load, VK_ATTACHMENT_LOAD_OP_LOAD )                                                              \
    X( RHI::ERHIRenderingLoadOp::Clear, VK_ATTACHMENT_LOAD_OP_CLEAR )                                                            \
    X( RHI::ERHIRenderingLoadOp::DontCare, VK_ATTACHMENT_LOAD_OP_DONT_CARE )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanAttachmentLoadOp,
                              RHI::ERHIRenderingLoadOp,
                              VkAttachmentLoadOp,
                              LUMEN_VK_ATTACHMENT_LOAD_OP_LIST,
                              RHI::MakeRHIInvalidCommand( "Invalid RHI::ERHIRenderingLoadOp value." ) )

#define LUMEN_VK_ATTACHMENT_STORE_OP_LIST( X )                                                                                   \
    X( RHI::ERHIRenderingStoreOp::Store, VK_ATTACHMENT_STORE_OP_STORE )                                                          \
    X( RHI::ERHIRenderingStoreOp::DontCare, VK_ATTACHMENT_STORE_OP_DONT_CARE )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanAttachmentStoreOp,
                              RHI::ERHIRenderingStoreOp,
                              VkAttachmentStoreOp,
                              LUMEN_VK_ATTACHMENT_STORE_OP_LIST,
                              RHI::MakeRHIInvalidCommand( "Invalid RHI::ERHIRenderingStoreOp value." ) )

    /**
     * Queries
     */

#define LUMEN_VK_QUERY_TYPE_LIST( X )                                                                                            \
    X( RHI::ERHIQueryType::Occlusion, VK_QUERY_TYPE_OCCLUSION )                                                                  \
    X( RHI::ERHIQueryType::Timestamp, VK_QUERY_TYPE_TIMESTAMP )                                                                  \
    X( RHI::ERHIQueryType::PipelineStatistics, VK_QUERY_TYPE_PIPELINE_STATISTICS )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanQueryType,
                              RHI::ERHIQueryType,
                              VkQueryType,
                              LUMEN_VK_QUERY_TYPE_LIST,
                              RHI::MakeRHIInvalidQuery( "Invalid RHI::ERHIQueryType value." ) )

    /**
     * Presentation (bidirectional)
     */

#define LUMEN_VK_PRESENT_MODE_LIST( X )                                                                                          \
    X( RHI::ERHIPresentMode::Fifo, VK_PRESENT_MODE_FIFO_KHR )                                                                    \
    X( RHI::ERHIPresentMode::Immediate, VK_PRESENT_MODE_IMMEDIATE_KHR )                                                          \
    X( RHI::ERHIPresentMode::Mailbox, VK_PRESENT_MODE_MAILBOX_KHR )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanPresentMode,
                              RHI::ERHIPresentMode,
                              VkPresentModeKHR,
                              LUMEN_VK_PRESENT_MODE_LIST,
                              RHI::MakeRHIInvalidPresentation( "Invalid RHI::ERHIPresentMode value." ) )
    LUMEN_VK_DEFINE_ENUM_UNMAP( ToRHIPresentMode, RHI::ERHIPresentMode, VkPresentModeKHR, LUMEN_VK_PRESENT_MODE_LIST )

#define LUMEN_VK_COLOR_SPACE_LIST( X )                                                                                           \
    X( RHI::ERHIColorSpace::SrgbNonLinear, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )                                                   \
    X( RHI::ERHIColorSpace::DisplayP3NonLinear, VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT )                                        \
    X( RHI::ERHIColorSpace::Hdr10St2084, VK_COLOR_SPACE_HDR10_ST2084_EXT )                                                       \
    X( RHI::ERHIColorSpace::ExtendedSrgbLinear, VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT )

    LUMEN_VK_DEFINE_ENUM_MAP( ToVulkanColorSpace,
                              RHI::ERHIColorSpace,
                              VkColorSpaceKHR,
                              LUMEN_VK_COLOR_SPACE_LIST,
                              RHI::MakeRHIInvalidPresentation( "Invalid RHI::ERHIColorSpace value." ) )
    LUMEN_VK_DEFINE_ENUM_UNMAP( ToRHIColorSpace, RHI::ERHIColorSpace, VkColorSpaceKHR, LUMEN_VK_COLOR_SPACE_LIST )

    /**
     * Queue compatibility (non-tabular: graphics & compute queues implicitly support transfers)
     */

    Bool IsVulkanQueueFamilyCompatible ( VkQueueFlags InQueueFlags, RHI::ERHIQueueType InQueueType ) noexcept
    {
        switch ( InQueueType )
        {
        case RHI::ERHIQueueType::Graphics:
            return ( InQueueFlags & VK_QUEUE_GRAPHICS_BIT ) != 0U;
        case RHI::ERHIQueueType::Compute:
            return ( InQueueFlags & VK_QUEUE_COMPUTE_BIT ) != 0U;
        case RHI::ERHIQueueType::Copy:
            return ( InQueueFlags & ( VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) ) != 0U;
        default:
            [[unlikely]] return false;
        }
    }

    /**
     * Cleanup
     */

#undef LUMEN_VK_ENUM_MAP_CASE
#undef LUMEN_VK_DEFINE_ENUM_MAP
#undef LUMEN_VK_ENUM_UNMAP_CASE
#undef LUMEN_VK_DEFINE_ENUM_UNMAP
#undef LUMEN_VK_FLAG_OR
#undef LUMEN_VK_FORMAT_LIST
#undef LUMEN_VK_IMAGE_ASPECT_LIST
#undef LUMEN_VK_BUFFER_USAGE_LIST
#undef LUMEN_VK_IMAGE_USAGE_LIST
#undef LUMEN_VK_BUFFER_STATE_LIST
#undef LUMEN_VK_BUFFER_STATE_ROW
#undef LUMEN_VK_SAMPLE_COUNT_LIST
#undef LUMEN_VK_SHADER_STAGE_LIST
#undef LUMEN_VK_DESCRIPTOR_TYPE_LIST
#undef LUMEN_VK_INDEX_TYPE_LIST
#undef LUMEN_VK_IMAGE_VIEW_TYPE_LIST
#undef LUMEN_VK_PRIMITIVE_TOPOLOGY_LIST
#undef LUMEN_VK_POLYGON_MODE_LIST
#undef LUMEN_VK_CULL_MODE_LIST
#undef LUMEN_VK_FRONT_FACE_LIST
#undef LUMEN_VK_BLEND_FACTOR_LIST
#undef LUMEN_VK_BLEND_OP_LIST
#undef LUMEN_VK_STENCIL_OP_LIST
#undef LUMEN_VK_COMPARE_OP_LIST
#undef LUMEN_VK_SAMPLER_FILTER_LIST
#undef LUMEN_VK_ADDRESS_MODE_LIST
#undef LUMEN_VK_ATTACHMENT_LOAD_OP_LIST
#undef LUMEN_VK_ATTACHMENT_STORE_OP_LIST
#undef LUMEN_VK_QUERY_TYPE_LIST
#undef LUMEN_VK_PRESENT_MODE_LIST
#undef LUMEN_VK_COLOR_SPACE_LIST

} // namespace RHIVulkan

} // namespace Lumen
