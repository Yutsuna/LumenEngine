/**
 * @file VulkanSwapChain.hpp
 * @brief Vulkan SwapChain RHI wrapper
 */

#pragma once

#include "Container/Vector.hpp"
#include "CoreTypes.hpp"
#include "Maths/Vec.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    class FVulkanQueue;
    class FVulkanDevice;
    class FVulkanSemaphore;
    class FVulkanPlatformWindowContext;

    /**
     * @enum ESwapChainMode
     * @brief Enumeration for different SwapChain presentation modes
     */
    enum class ESwapChainMode
    {
        /** No VSync, may cause tearing */
        Immediate,
        /** VSync enabled, synchronizes with the display's refresh rate */
        Mailbox,
        /** VSync enabled, but may allow for some tearing if the display can't keep up */
        Fifo,
        /** VSync enabled, but may allow for some tearing if the display can't keep up, and may allow for lower latency than Fifo */
        FifoRelaxed
    };

    /**
     * @enum ESwapChainStatus
     * @brief Enumeration for the status of the SwapChain
     */
    enum class ESwapChainStatus
    {
        Heatlhy     = 0,
        OutOfDate   = 1,
        SurfaceLost = 2,
    };

    /**
     * @class FVulkanSwapChain
     * @brief A wrapper class for Vulkan SwapChain, managing the presentation of rendered images to the screen.
     */
    class FVulkanSwapChain final
    {
    public:

        FVulkanSwapChain ( VkInstance InInstance,
                           FVulkanDevice &InDevice,
                           VkSurfaceKHR InSurface,
                           VkSwapchainKHR InSwapChain,
                           const VkSurfaceTransformFlagBitsKHR PreTransform,
                           const Maths::FVec2u &InSize,
                           const Bool InIsFullscreen,
                           const UInt32 NumSwapChainImages,
                           const Int8 InLockToVsync,
                           FVulkanPlatformWindowContext &WindowContext );

        ~FVulkanSwapChain () noexcept = default;

    public:

        /**
         * @brief Presents the current image to the screen
         * @param PresentQueue to submit the present operation to
         * @param BackBufferRenderingDoneSemaphore signals when the back buffer rendering is done
         * @return ESwapChainStatus indicating the result of the present operation
         */
        ESwapChainStatus Present ( FVulkanQueue *PresentQueue, FVulkanSemaphore *BakBufferRenderingDoneSemaphore );

    private:

        /**
         * @brief Acquires the next available image from the swap chain for rendering
         * @param InSemaphore to signal when the image is acquired
         * @param InFence to signal when the image is acquired (optional)
         * @return The index of the acquired image in the swap chain
         */
        UInt32 AcquireNextImage ( VkSemaphore InSemaphore, VkFence InFence = VK_NULL_HANDLE );

        void SwapChainValidation ();

    private:

        static constexpr UInt32 InvalidSwapChainImageIndex = static_cast<UInt32>( -1 );

        struct FCoreContext
        {
            const VkInstance Instance;
            FVulkanDevice &Device;
            VkSurfaceKHR Surface = VK_NULL_HANDLE;
            void *WindowHandle   = nullptr;

            inline FCoreContext ( const VkInstance InInst, FVulkanDevice &InDev ) : Instance( InInst ), Device( InDev )
            {
                /* Ctor */
            }
        } Core;

        struct FGeometry final
        {
            Maths::FVec2u Size = { .Width = 0, .Height = 0 };
            VkSurfaceTransformFlagBitsKHR PreTransform;
            bool bIsFullScreen = false;
            Int8 LockToVsync   = 0;
        } Geometry;

        struct FState final
        {
            VkSwapchainKHR Handle    = VK_NULL_HANDLE;
            UInt32 CurrentImageIndex = InvalidSwapChainImageIndex;
            Int32 SemaphoreIndex     = 0;
            UInt32 NumPresentCalls   = 0;
            UInt32 NumAcquireCalls   = 0;
            UInt32 PresentID         = 0;
        } State;

        struct FPacingData final
        {
            UInt32 SampleCount            = 0;
            Float64 PreviousFrameCPUTime  = 0;
            Float64 SampledDeltaTimeMS    = 0;
            Float64 NextPresentTargetTime = 0;
        } Pacing;

        struct FSyncResources final
        {
            TVector<FVulkanSemaphore *> ImageAcquiredSemaphore;
        } Sync;
    };

} // namespace RHI

} // namespace LumenEngine
