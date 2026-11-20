/**
 * @file VulkanHandle.hpp
 * @brief Wrapper around Vulkan handle types
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @class TVulkanScopedHandle
     * @brief Wrapper around Vulkan handle types that automatically destroys the handle when the object is destroyed.
     * @tparam THandleType The type of the Vulkan handle.
     * @tparam TDeleterType The type of the deleter function.
     */
    template <class THandleType, class TDeleterType> class TVulkanScopedHandle final : public FNonCopyable
    {
    public:

        TVulkanScopedHandle () noexcept = default;
        explicit TVulkanScopedHandle ( THandleType InHandle, TDeleterType InDeleter = {} ) noexcept;

        ~TVulkanScopedHandle () noexcept;

    public:

        TVulkanScopedHandle ( TVulkanScopedHandle &&InOther ) noexcept;
        TVulkanScopedHandle &operator=( TVulkanScopedHandle &&InOther ) noexcept;

    public:

        /** @brief Destroys the currentlyl owned handle if valid and adopts InNewHandle. */
        void Reset ( THandleType InNewHandle = VK_NULL_HANDLE ) noexcept;

        /** @brief Destroys any current handle and returns a pointer for native creation out-params. */
        [[nodiscard]] THandleType *ResetAndGetAddressOf ( TDeleterType InDeleter = {} ) noexcept;

        /** @brief Releases ownership without destroying the  handle. */
        [[nodiscard]] THandleType Release () noexcept;

        /** @return The owned native handle, or VK_NULL_HANDLE if none. */
        [[nodiscard]] THandleType Get () const noexcept;

        /** @return Reference for the attached deleter state. */
        [[nodiscard]] const TDeleterType &GetDeleter () const noexcept;

        /** @return True when the owned handle is not VK_HANDLE. */
        [[nodiscard]] Bool IsValid () const noexcept;

        /** @return True when valid. */
        [[nodiscard]] explicit operator Bool () const noexcept;

    private:

        THandleType Handle   = VK_NULL_HANDLE;
        TDeleterType Deleter = {};
    };

    /**
     * @struct FVulkanInstanceDeleter
     * @brief Deleter function for Vulkan instance handles.
     */
    struct FVulkanInstanceDeleter
    {
        void operator()( VkInstance InHandle ) const noexcept;
    };

    /**
     * @struct FVulkanDeviceDeleter
     * @brief Deleter function for Vulkan device handles.
     */
    struct FVulkanDeviceDeleter
    {
        void operator()( VkDevice InHandle ) const noexcept;
    };

    /**
     * @struct FVulkanDebugMessengerDeleter
     * @brief Deleter function for Vulkan debug messenger handles.
     */
    struct FVulkanDebugMessengerDeleter
    {
        VkInstance Instance                           = VK_NULL_HANDLE;
        PFN_vkDestroyDebugUtilsMessengerEXT DestroyFn = nullptr;

        void operator()( VkDebugUtilsMessengerEXT InHandle ) const noexcept;
    };

    /**
     * @struct FVulkanSwapchainDeleter
     * @brief Deleter function for Vulkan swapchain handles.
     */
    struct FVulkanSwapchainDeleter
    {
        VkDevice Device                     = VK_NULL_HANDLE;
        PFN_vkDestroySwapchainKHR DestroyFn = nullptr;

        void operator()( VkSwapchainKHR InHandle ) const noexcept;
    };

#define LUMEN_VK_DEVICE_CHILD_DELETER( Name, HandleType, DestroyFn )                                                             \
    struct Name                                                                                                                  \
    {                                                                                                                            \
        VkDevice Device = VK_NULL_HANDLE;                                                                                        \
        void operator()( HandleType InHandle ) const noexcept                                                                    \
        {                                                                                                                        \
            if ( Device != VK_NULL_HANDLE and InHandle != VK_NULL_HANDLE )                                                       \
            {                                                                                                                    \
                DestroyFn( Device, InHandle, nullptr );                                                                          \
            }                                                                                                                    \
        }                                                                                                                        \
    }

    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanBufferDeleter, VkBuffer, vkDestroyBuffer );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanImageDeleter, VkImage, vkDestroyImage );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanImageViewDeleter, VkImageView, vkDestroyImageView );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanSamplerDeleter, VkSampler, vkDestroySampler );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanDeviceMemoryDeleter, VkDeviceMemory, vkFreeMemory );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanDescriptorSetLayoutDeleter, VkDescriptorSetLayout, vkDestroyDescriptorSetLayout );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanDescriptorPoolDeleter, VkDescriptorPool, vkDestroyDescriptorPool );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanPipelineLayoutDeleter, VkPipelineLayout, vkDestroyPipelineLayout );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanPipelineDeleter, VkPipeline, vkDestroyPipeline );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanPipelineCacheDeleter, VkPipelineCache, vkDestroyPipelineCache );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanShaderModuleDeleter, VkShaderModule, vkDestroyShaderModule );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanCommandPoolDeleter, VkCommandPool, vkDestroyCommandPool );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanSemaphoreDeleter, VkSemaphore, vkDestroySemaphore );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanFenceDeleter, VkFence, vkDestroyFence );
    LUMEN_VK_DEVICE_CHILD_DELETER( FVulkanQueryPoolDeleter, VkQueryPool, vkDestroyQueryPool );

#undef LUMEN_VK_DEVICE_CHILD_DELETER

    using FVulkanInstanceHandle            = TVulkanScopedHandle<VkInstance, FVulkanInstanceDeleter>;
    using FVulkanDeviceHandle              = TVulkanScopedHandle<VkDevice, FVulkanDeviceDeleter>;
    using FVulkanDebugMessengerHandle      = TVulkanScopedHandle<VkDebugUtilsMessengerEXT, FVulkanDebugMessengerDeleter>;
    using FVulkanSwapchainHandle           = TVulkanScopedHandle<VkSwapchainKHR, FVulkanSwapchainDeleter>;
    using FVulkanBufferHandle              = TVulkanScopedHandle<VkBuffer, FVulkanBufferDeleter>;
    using FVulkanImageHandle               = TVulkanScopedHandle<VkImage, FVulkanImageDeleter>;
    using FVulkanImageViewHandle           = TVulkanScopedHandle<VkImageView, FVulkanImageViewDeleter>;
    using FVulkanSamplerHandle             = TVulkanScopedHandle<VkSampler, FVulkanSamplerDeleter>;
    using FVulkanDeviceMemoryHandle        = TVulkanScopedHandle<VkDeviceMemory, FVulkanDeviceMemoryDeleter>;
    using FVulkanDescriptorSetLayoutHandle = TVulkanScopedHandle<VkDescriptorSetLayout, FVulkanDescriptorSetLayoutDeleter>;
    using FVulkanDescriptorPoolHandle      = TVulkanScopedHandle<VkDescriptorPool, FVulkanDescriptorPoolDeleter>;
    using FVulkanPipelineLayoutHandle      = TVulkanScopedHandle<VkPipelineLayout, FVulkanPipelineLayoutDeleter>;
    using FVulkanPipelineHandle            = TVulkanScopedHandle<VkPipeline, FVulkanPipelineDeleter>;
    using FVulkanPipelineCacheHandle       = TVulkanScopedHandle<VkPipelineCache, FVulkanPipelineCacheDeleter>;
    using FVulkanShaderModuleHandle        = TVulkanScopedHandle<VkShaderModule, FVulkanShaderModuleDeleter>;
    using FVulkanCommandPoolHandle         = TVulkanScopedHandle<VkCommandPool, FVulkanCommandPoolDeleter>;
    using FVulkanSemaphoreHandle           = TVulkanScopedHandle<VkSemaphore, FVulkanSemaphoreDeleter>;
    using FVulkanFenceHandle               = TVulkanScopedHandle<VkFence, FVulkanFenceDeleter>;
    using FVulkanQueryPoolHandle           = TVulkanScopedHandle<VkQueryPool, FVulkanQueryPoolDeleter>;

} // namespace RHIVulkan

} // namespace Lumen

#include "Inline/VulkanHandle.inl"
