/**
 * @file VulkanSwapChain.cpp
 * @brief Implementation of the Vulkan SwapChain wrapper class
 */

#include "VulkanSwapChain.hpp"
#include "CoreTypes.hpp"
#include "Logging/Logger.hpp"
#include "Logging/LoggingCategory.hpp"

LUMEN_LOG_DEFINE_CATEGORY( LogVulkanRHI, "LogVulkanRHI" );

LumenEngine::RHI::FVulkanSwapChain::FVulkanSwapChain ( const VkInstance InInstance,
                                                       FVulkanDevice &InDevice,
                                                       VkSurfaceKHR InSurface,
                                                       VkSwapchainKHR InSwapChain,
                                                       const VkSurfaceTransformFlagBitsKHR InPreTransform,
                                                       const Maths::FVec2u &InSize,
                                                       const Bool InIsFullscreen,
                                                       const UInt32 InNumSwapChainImages,
                                                       const Int8 InLockToVsync,
                                                       FVulkanPlatformWindowContext &InWindowContext )
    : Core( InInstance, InDevice ), Geometry( InSize, InPreTransform, InIsFullscreen, InLockToVsync ), State(), Pacing(), Sync()
{
    Core.Surface = InSurface;
    State.Handle = InSwapChain;

    Sync.ImageAcquiredSemaphore.reserve( InNumSwapChainImages );
    SwapChainValidation();
}

LumenEngine::UInt32 LumenEngine::RHI::FVulkanSwapChain::AcquireNextImage ( VkSemaphore InSemaphore, VkFence InFence )
{
    if ( State.CurrentImageIndex == InvalidSwapChainImageIndex )
    {
        LUMEN_LOG_WARNING( LogVulkanRHI, "Attempting to acquire next image while a valid image is already acquired. This may indicate a synchronization issue." );
        return InvalidSwapChainImageIndex;
    }

    UInt32 ImageIndex              = 0;
    const Int32 PrevSemaphoreIndex = State.SemaphoreIndex;

    State.SemaphoreIndex = ( State.SemaphoreIndex + 1 ) % Sync.ImageAcquiredSemaphore.capacity();

    if ( not State.NumAcquireCalls == Sync.ImageAcquiredSemaphore.capacity() - 1 && State.NumPresentCalls == 0 )
    {
        LUMEN_LOG_WARNING( LogVulkanRHI,
                           "AcquireNextImage called more times than the number of swap chain images without presenting. This may indicate a synchronization issue." );
        return InvalidSwapChainImageIndex;
    }
}

void LumenEngine::RHI::FVulkanSwapChain::SwapChainValidation ()
{
    if ( Core.Instance == VK_NULL_HANDLE )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Invalid Vulkan instance provided to FVulkanSwapChain constructor" );
    }
    if ( Core.Device.GetDevice() == VK_NULL_HANDLE )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Invalid Vulkan device provided to FVulkanSwapChain constructor" );
    }
    if ( Core.Surface == VK_NULL_HANDLE )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Invalid Vulkan surface provided to FVulkanSwapChain constructor" );
    }
    if ( State.Handle == VK_NULL_HANDLE )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Invalid Vulkan swap chain provided to FVulkanSwapChain constructor" );
    }
}
