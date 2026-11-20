/**
 * @file VulkanTypeMapping.hpp
 * @brief Declares portable RHI-to-Vulkan enum, format, usage, and state mappings.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <RHI/Bindings/RHIBindingType.hpp>
#include <RHI/Commands/RHIDrawArguments.hpp>
#include <RHI/Commands/RHIRenderingDesc.hpp>
#include <RHI/Pipelines/RHIBlendState.hpp>
#include <RHI/Pipelines/RHIDepthStencilState.hpp>
#include <RHI/Pipelines/RHIPrimitiveTopology.hpp>
#include <RHI/Pipelines/RHIRasterizerState.hpp>
#include <RHI/Presentation/RHIColorSpace.hpp>
#include <RHI/Presentation/RHIPresentMode.hpp>
#include <RHI/Queries/RHIQueryType.hpp>
#include <RHI/Resources/RHIResourceUsage.hpp>
#include <RHI/Resources/RHISamplerDesc.hpp>
#include <RHI/Resources/RHITextureViewDesc.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIFormat.hpp>
#include <RHI/Types/RHIFormatInfo.hpp>
#include <RHI/Types/RHIQueueType.hpp>
#include <RHI/Types/RHIResourceState.hpp>
#include <RHI/Types/RHISampleCount.hpp>
#include <RHI/Types/RHIShaderStage.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

#include <vulkan/vulkan.h>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @struct FVulkanResourceBarrierState
     * @brief Synchronization2 stage/access/layout tuple derived from ERHIResourceState.
     */
    struct FVulkanResourceBarrierState
    {
        VkPipelineStageFlags2 StageMask = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 AccessMask       = VK_ACCESS_2_NONE;
        VkImageLayout ImageLayout       = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    /** @return True when InFormat carries a depth or stencil plane. */
    [[nodiscard]] constexpr Bool IsVulkanDepthStencilFormat ( VkFormat InFormat ) noexcept;

    /** @brief Maps a non-Unknown RHI format to VkFormat. */
    [[nodiscard]] TExpected<VkFormat, RHI::FRHIError> ToVulkanFormat ( RHI::ERHIFormat InFormat ) noexcept;

    /** @brief Maps any RHI format (including Unknown -> VK_FORMAT_UNDEFINED) to VkFormat. */
    [[nodiscard]] TExpected<VkFormat, RHI::FRHIError> ToVulkanFormatOrUndefined ( RHI::ERHIFormat InFormat ) noexcept;

    /** @return Portable RHI format for a VkFormat, or ERHIFormat::Unknown if unmapped. */
    [[nodiscard]] RHI::ERHIFormat ToRHIFormat ( VkFormat InFormat ) noexcept;

    /** @brief Maps explicit format aspect flags to VkImageAspectFlags. */
    [[nodiscard]] TExpected<VkImageAspectFlags, RHI::FRHIError> ToVulkanImageAspect ( RHI::ERHIFormatAspect InAspect ) noexcept;

    /** @brief Computes all image aspects owned by InFormat. */
    [[nodiscard]] TExpected<VkImageAspectFlags, RHI::FRHIError> GetVulkanFullImageAspect ( RHI::ERHIFormat InFormat ) noexcept;

    /** @brief Maps resource usage flags to VkBufferUsageFlags, rejecting image-only usages. */
    [[nodiscard]] TExpected<VkBufferUsageFlags, RHI::FRHIError> ToVulkanBufferUsage ( RHI::ERHIResourceUsage InUsage ) noexcept;

    /** @brief Maps resource usage flags to VkImageUsageFlags, rejecting buffer-only usages. */
    [[nodiscard]] TExpected<VkImageUsageFlags, RHI::FRHIError> ToVulkanImageUsage ( RHI::ERHIResourceUsage InUsage ) noexcept;

    /** @brief Maps buffer access state flags to Synchronization2 stage/access masks. */
    [[nodiscard]] TExpected<FVulkanResourceBarrierState, RHI::FRHIError>
    ToVulkanBufferState ( RHI::ERHIResourceState InState ) noexcept;

    /** @brief Maps image access state flags to Synchronization2 stage/access/layout values. */
    [[nodiscard]] TExpected<FVulkanResourceBarrierState, RHI::FRHIError>
    ToVulkanImageState ( RHI::ERHIResourceState InState, Bool bIsDepthStencil = false ) noexcept;

    /** @brief Maps sample count to VkSampleCountFlagBits. */
    [[nodiscard]] TExpected<VkSampleCountFlagBits, RHI::FRHIError> ToVulkanSampleCount ( RHI::ERHISampleCount InCount ) noexcept;

    /** @brief Maps shader stage bitmask to VkShaderStageFlags. */
    [[nodiscard]] TExpected<VkShaderStageFlags, RHI::FRHIError>
    ToVulkanShaderStageFlags ( RHI::ERHIShaderStage InStages ) noexcept;

    /** @brief Maps exactly one shader stage to VkShaderStageFlagBits. */
    [[nodiscard]] TExpected<VkShaderStageFlagBits, RHI::FRHIError>
    ToVulkanSingleShaderStage ( RHI::ERHIShaderStage InStage ) noexcept;

    /** @brief Maps portable descriptor binding type to VkDescriptorType. */
    [[nodiscard]] TExpected<VkDescriptorType, RHI::FRHIError> ToVulkanDescriptorType ( RHI::ERHIBindingType InType ) noexcept;

    /** @brief Maps index element format to VkIndexType. */
    [[nodiscard]] TExpected<VkIndexType, RHI::FRHIError> ToVulkanIndexType ( RHI::ERHIIndexFormat InFormat ) noexcept;

    /** @brief Maps texture view dimension to VkImageViewType. */
    [[nodiscard]] TExpected<VkImageViewType, RHI::FRHIError> ToVulkanImageViewType ( RHI::ERHITextureViewType InType ) noexcept;

    /** @brief Maps primitive topology to VkPrimitiveTopology. */
    [[nodiscard]] TExpected<VkPrimitiveTopology, RHI::FRHIError>
    ToVulkanPrimitiveTopology ( RHI::ERHIPrimitiveTopology InTopology ) noexcept;

    /** @brief Maps polygon fill mode to VkPolygonMode. */
    [[nodiscard]] TExpected<VkPolygonMode, RHI::FRHIError> ToVulkanPolygonMode ( RHI::ERHIFillMode InFillMode ) noexcept;

    /** @brief Maps face culling mode to VkCullModeFlags. */
    [[nodiscard]] TExpected<VkCullModeFlags, RHI::FRHIError> ToVulkanCullMode ( RHI::ERHICullMode InCullMode ) noexcept;

    /** @brief Maps front face winding to VkFrontFace. */
    [[nodiscard]] TExpected<VkFrontFace, RHI::FRHIError> ToVulkanFrontFace ( RHI::ERHIFrontFace InFrontFace ) noexcept;

    /** @brief Maps blend factor to VkBlendFactor. */
    [[nodiscard]] TExpected<VkBlendFactor, RHI::FRHIError> ToVulkanBlendFactor ( RHI::ERHIBlendFactor InFactor ) noexcept;

    /** @brief Maps blend op to VkBlendOp. */
    [[nodiscard]] TExpected<VkBlendOp, RHI::FRHIError> ToVulkanBlendOp ( RHI::ERHIBlendOp InOp ) noexcept;

    /** @brief Maps stencil op to VkStencilOp. */
    [[nodiscard]] TExpected<VkStencilOp, RHI::FRHIError> ToVulkanStencilOp ( RHI::ERHIStencilOp InOp ) noexcept;

    /** @brief Maps comparison function to VkCompareOp. */
    [[nodiscard]] TExpected<VkCompareOp, RHI::FRHIError> ToVulkanCompareOp ( RHI::ERHICompareFunction InCompare ) noexcept;

    /** @brief Maps sampler filter to VkFilter. */
    [[nodiscard]] TExpected<VkFilter, RHI::FRHIError> ToVulkanFilter ( RHI::ERHISamplerFilter InFilter ) noexcept;

    /** @brief Maps sampler mip filter to VkSamplerMipmapMode. */
    [[nodiscard]] TExpected<VkSamplerMipmapMode, RHI::FRHIError> ToVulkanMipmapMode ( RHI::ERHISamplerFilter InFilter ) noexcept;

    /** @brief Maps sampler address mode to VkSamplerAddressMode. */
    [[nodiscard]] TExpected<VkSamplerAddressMode, RHI::FRHIError>
    ToVulkanAddressMode ( RHI::ERHISamplerAddressMode InMode ) noexcept;

    /** @brief Maps attachment load op to VkAttachmentLoadOp. */
    [[nodiscard]] TExpected<VkAttachmentLoadOp, RHI::FRHIError>
    ToVulkanAttachmentLoadOp ( RHI::ERHIRenderingLoadOp InOp ) noexcept;

    /** @brief Maps attachment store op to VkAttachmentStoreOp. */
    [[nodiscard]] TExpected<VkAttachmentStoreOp, RHI::FRHIError>
    ToVulkanAttachmentStoreOp ( RHI::ERHIRenderingStoreOp InOp ) noexcept;

    /** @brief Maps query type to VkQueryType. */
    [[nodiscard]] TExpected<VkQueryType, RHI::FRHIError> ToVulkanQueryType ( RHI::ERHIQueryType InType ) noexcept;

    /** @brief Maps present mode to VkPresentModeKHR. */
    [[nodiscard]] TExpected<VkPresentModeKHR, RHI::FRHIError> ToVulkanPresentMode ( RHI::ERHIPresentMode InMode ) noexcept;

    /** @return Portable present mode for a VkPresentModeKHR, or empty when unsupported. */
    [[nodiscard]] TOptional<RHI::ERHIPresentMode> ToRHIPresentMode ( VkPresentModeKHR InMode ) noexcept;

    /** @brief Maps swapchain color space to VkColorSpaceKHR. */
    [[nodiscard]] TExpected<VkColorSpaceKHR, RHI::FRHIError> ToVulkanColorSpace ( RHI::ERHIColorSpace InColorSpace ) noexcept;

    /** @return Portable color space for a VkColorSpaceKHR, or empty when unsupported. */
    [[nodiscard]] TOptional<RHI::ERHIColorSpace> ToRHIColorSpace ( VkColorSpaceKHR InColorSpace ) noexcept;

    /** @return True when InQueueFlags satisfies the work class required by InQueueType. */
    [[nodiscard]] Bool IsVulkanQueueFamilyCompatible ( VkQueueFlags InQueueFlags, RHI::ERHIQueueType InQueueType ) noexcept;

} // namespace RHIVulkan

} // namespace Lumen

#include "Inline/VulkanTypeMapping.inl"
