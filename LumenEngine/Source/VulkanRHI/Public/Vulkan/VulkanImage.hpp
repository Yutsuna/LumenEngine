/**
 * @file VulkanImage.hpp
 * @brief Vulkan image wrapper for resource management
 */

#pragma once

#include "Container/String.hpp"
#include "CoreTypes.hpp"

#include "Maths/Vec.hpp"
#include "Vulkan/VulkanBuffer.hpp"
#include "Vulkan/VulkanCore.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @struct FVulkanImage
     * @brief Wrapper around a Vulkan Image and its VMA Allocation
     */
    struct FVulkanImage final
    {
    public:

        VkImage Image            = VK_NULL_HANDLE;
        VkImageView ImageView    = VK_NULL_HANDLE;
        VmaAllocation Allocation = VK_NULL_HANDLE;
        VkFormat Format          = VK_FORMAT_UNDEFINED;
        VkImageUsageFlags Usage  = 0;
        VkExtent3D Extent        = {};

        UInt32 MipLevels = 1;
        UInt32 NumLayers = 1;
        Bool bIsCubemap  = false;
        FString DebugName;

        [[nodiscard]] Maths::FVec2i GetSize2D () const noexcept;
        [[nodiscard]] VkExtent2D GetExtent2D () const noexcept;
        [[nodiscard]] UInt32 GetBindlessID () const noexcept;
        void SetBindlessID ( const UInt32 InID ) noexcept;
        [[nodiscard]] Bool IsInitialized () const noexcept;

    private:

        UInt32 BindlessID = NullBindlessID;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
