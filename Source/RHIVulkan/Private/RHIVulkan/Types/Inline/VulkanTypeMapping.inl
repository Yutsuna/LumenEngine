/**
 * @file VulkanTypeMapping.inl
 * @brief Implements inline Vulkan type classification helpers.
 */

#pragma once

#include <RHIVulkan/Types/VulkanTypeMapping.hpp>

constexpr Lumen::Bool Lumen::RHIVulkan::IsVulkanDepthStencilFormat ( VkFormat InFormat ) noexcept
{
    switch ( InFormat )
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    default:
        return false;
    }
}
