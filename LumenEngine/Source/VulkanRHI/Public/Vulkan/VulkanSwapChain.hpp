/**
 * @file VulkanSwapChain.hpp
 * @brief Declaration of the FVulkanSwapChain class for Vulkan presentation.
 */

#pragma once

#include "Container/Vector.hpp"
#include "Definitions.hpp"
#include "Maths/Vec.hpp"

#include "Vulkan/VulkanCore.hpp"

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @struct FFrameData
     * @brief Synchronization primitives for a single frame.
     */
    struct FFrameData final
    {
        VkSemaphore SwapChainSemaphore = VK_NULL_HANDLE;
        VkFence RenderFence            = VK_NULL_HANDLE;
    };

    /**
     * @struct FSwapChainDescription
     * @brief Logical parameters describing swapchain creation.
     */
    struct FSwapChainDescription final
    {
        VkSurfaceKHR Surface = VK_NULL_HANDLE;
        VkFormat Format      = VK_FORMAT_B8G8R8A8_SRGB;
        Maths::FVec2u Size;
        Bool bVSyncEnabled = true;
    };

    /**
     * @class FVulkanSwapChain
     * @brief Manages the Vulkan SwapChain and frame Synchronization
     */
    class LUMEN_ENGINE_API FVulkanSwapChain
    {
    public:

        FVulkanSwapChain ()  = default;
        ~FVulkanSwapChain () = default;

    public:

        /**
         * @brief Initializes synchronization primitives for the swap chain.
         * @param InDevice The Vulkan device to create synchronization objects with.
         */
        void InitializeSynStructures ( VkDevice InDevice );

        /**
         * @brief Creates the Vulkan SwapChain with the specified parameters natively.
         */
        void Create (
            VkPhysicalDevice InPhysicalDevice, VkDevice InDevice, VkSurfaceKHR InSurface, VkFormat InSwapChainFormat, const Maths::FVec2u &InSize, bool bInVSyncEnabled );

        /**
         * @brief Recreates the Vulkan SwapChain natively.
         */
        void Recreate (
            VkPhysicalDevice InPhysicalDevice, VkDevice InDevice, VkSurfaceKHR InSurface, VkFormat InSwapChainFormat, const Maths::FVec2u &InSize, bool bInVSyncEnabled );

        /**
         * @brief Cleans up Vulkan resources associated with the swap chain.
         * @param InDevice The Vulkan device to clean up resources from.
         */
        void Cleanup ( VkDevice InDevice ) noexcept;

    public:

        [[nodiscard]] VkExtent2D GetExtent () const noexcept;
        [[nodiscard]] const TVector<VkImage> &GetImages () const noexcept;
        [[nodiscard]] VkImageView GetImageView ( const USize InImageIndex ) const noexcept;
        [[nodiscard]] VkFormat GetImageFormat () const noexcept;

        [[nodiscard]] Bool NeedsRecreation () const noexcept;

    public:

        void BeginFrame ( VkDevice InDevice, USize InFrameIndex ) const;
        void ResetFences ( VkDevice InDevice, USize InFrameIndex ) const;
        [[nodiscard]] std::pair<VkImage, UInt32> AcquireNextImage ( VkDevice InDevice, USize InFrameIndex );
        void SubmitAndPresent ( VkCommandBuffer InCmd, VkQueue InGraphicsQueue, USize InFrameIndex, UInt32 InSwapChainImageIndex ) noexcept;

    private:

        void CreateInternal ( VkPhysicalDevice InPhysicalDevice, VkDevice InDevice, const FSwapChainDescription &InDescription, VkSwapchainKHR InOldSwapchainHandle );

    private:

        VkSwapchainKHR SwapChainHandle = VK_NULL_HANDLE;
        VkExtent2D Extent              = {};
        VkFormat ImageFormat           = VK_FORMAT_UNDEFINED;

        TVector<VkImage> Images;
        TVector<VkImageView> ImageViews;
        TVector<VkSemaphore> RenderSemaphores;

        FFrameData Frames[MaxFramesInFlight] = {};
        Bool bIsDirty                        = false;
    };

} // namespace VulkanRHI

} // namespace LumenEngine