/**
 * @file VulkanRenderContextManager.hpp
 * @brief Helper utility functions to configure rendering contexts and pipeline barriers.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Vulkan/VulkanMsaaManager.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    namespace VulkanRenderContextManager
    {
        /**
         * @brief Starts a dynamic rendering command sequence inside a given command buffer.
         * Handles MSAA resolve targets or single-sample present images automatically.
         */
        void BeginRendering ( VkCommandBuffer InCmd,
                              VkImage InPresentImage,
                              VkImageView InPresentView,
                              VkFormat InFormat,
                              VkExtent2D InExtent,
                              const FVulkanMsaaManager &InMsaaManager,
                              const Float32 InClearColor[4] ) noexcept;

        /**
         * @brief Transition presentation image to presentable format layout.
         */
        void TransitionPresentImageToPresentSource ( VkCommandBuffer InCmd, VkImage InPresentImage ) noexcept;

    } // namespace VulkanRenderContextManager

} // namespace VulkanRHI

} // namespace LumenEngine
