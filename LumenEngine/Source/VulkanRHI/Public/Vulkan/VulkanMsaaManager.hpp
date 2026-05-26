/**
 * @file VulkanMsaaManager.hpp
 * @brief Declaration of the FVulkanMsaaManager class for managing MSAA render targets.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "RHI/RHITypes.hpp"
#include "RHI/ResourceRegistry.hpp"

#include "Vulkan/VulkanDeferredDestruction.hpp"
#include "Vulkan/VulkanMsaaRenderTarget.hpp"
#include "Vulkan/VulkanPipeline.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FVulkanMsaaManager
     * @brief Manages MSAA sample counts, transient MSAA render targets, and pipeline recreations.
     */
    class LUMEN_ENGINE_API FVulkanMsaaManager final
    {
    public:

        FVulkanMsaaManager () noexcept  = default;
        ~FVulkanMsaaManager () noexcept = default;

    public:

        /**
         * @brief Initializes the MSAA manager and creates initial render targets
         */
        void Initialize ( VkPhysicalDevice InPhysicalDevice,
                          VmaAllocator InAllocator,
                          VkDevice InDevice,
                          VkFormat InFormat,
                          VkExtent2D InExtent,
                          const RHI::FVisualSettings &InSettings );

        /**
         * @brief Cleans up all MSAA-related resources.
         */
        void Shutdown ( VmaAllocator InAllocator, VkDevice InDevice ) noexcept;

        /**
         * @brief Dynamically updates visual settings, recreating MSAA targets and pipelines if settings change
         */
        void SetVisualSettings ( const RHI::FVisualSettings &InSettings,
                                 VkPhysicalDevice InPhysicalDevice,
                                 VkDevice InDevice,
                                 VmaAllocator InAllocator,
                                 VkFormat InFormat,
                                 VkExtent2D InExtent,
                                 FDeferredDestructionQueue &InDestructionQueue,
                                 RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                                 UInt64 InAbsoluteFrameIndex );

        /**
         * @brief Helper to recreate MSAA render targets dynamically on window/viewport resize.
         */
        void RecreateRenderTargetIfNeeded ( VmaAllocator InAllocator, VkDevice InDevice, VkFormat InFormat, VkExtent2D InExtent );

    public:

        [[nodiscard]] VkSampleCountFlagBits GetActiveSamples () const noexcept;
        [[nodiscard]] const FVulkanMsaaRenderTarget &GetRenderTarget () const noexcept;
        [[nodiscard]] const RHI::FVisualSettings &GetCurrentSettings () const noexcept;

    private:

        void UpdateMsaaSamples ( VkPhysicalDevice InPhysicalDevice ) noexcept;

    private:

        RHI::FVisualSettings CurrentSettings;
        VkSampleCountFlagBits ActiveMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
        FVulkanMsaaRenderTarget MsaaRenderTarget;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
