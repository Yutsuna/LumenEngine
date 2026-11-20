/**
 * @file VulkanHandle.cpp
 * @brief Vulkan handle management implementation.
 */

#include "VulkanHandle.hpp"

void Lumen::RHIVulkan::FVulkanInstanceDeleter::operator()( VkInstance InHandle ) const noexcept
{
    if ( InHandle != VK_NULL_HANDLE ) [[likely]]
    {
        vkDestroyInstance( InHandle, nullptr );
    }
}

void Lumen::RHIVulkan::FVulkanDeviceDeleter::operator()( VkDevice InHandle ) const noexcept
{
    if ( InHandle != VK_NULL_HANDLE ) [[likely]]
    {
        vkDestroyDevice( InHandle, nullptr );
    }
}

void Lumen::RHIVulkan::FVulkanDebugMessengerDeleter::operator()( VkDebugUtilsMessengerEXT InHandle ) const noexcept
{
    if ( Instance != VK_NULL_HANDLE and DestroyFn != nullptr and InHandle != VK_NULL_HANDLE ) [[likely]]
    {
        DestroyFn( Instance, InHandle, nullptr );
    }
}

void Lumen::RHIVulkan::FVulkanSwapchainDeleter::operator()( VkSwapchainKHR InHandle ) const noexcept
{
    if ( Device != VK_NULL_HANDLE and DestroyFn != nullptr and InHandle != VK_NULL_HANDLE ) [[likely]]
    {
        DestroyFn( Device, InHandle, nullptr );
    }
}
