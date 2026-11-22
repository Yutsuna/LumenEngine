/**
 * @file VulkanDevice.hpp
 * @brief Declares the Vulkan IRHIDevice implementation built on FRHIDeviceBase.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Device/RHIDeviceBase.hpp>
#include <RHI/Module/RHIDeviceDesc.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>

#include <RHIVulkan/Device/VulkanDeviceContext.hpp>
#include <RHIVulkan/Module/VulkanAdapter.hpp>
#include <RHIVulkan/Types/VulkanObject.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    class FVulkanInstance;
    class FVulkanQueue;

    /**
     * @class FVulkanDevice
     * @brief Logical Vulkan device implementing object tracking, status propagation, and factories.
     */
    class FVulkanDevice final : public RHI::FRHIDeviceBase, public FNonCopyable, public FNonMovable
    {
    public:

        /** @brief Negotiates features, creates VkDevice and requested queues, and returns the device. */
        [[nodiscard]] static TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Create (
            TSharedRef<FVulkanInstance> InInstance, const FVulkanAdapter &InAdapter, const RHI::FRHIDeviceDesc &InDesc ) noexcept;

        ~FVulkanDevice () noexcept override;

        /** @brief Transitions the device and shared context to DeviceLost for conformance testing. */
        void InjectDeviceLost () noexcept;

        /** @brief Returns the shared native device context. */
        [[nodiscard]] const TSharedRef<FVulkanDeviceContext> &GetContext () const noexcept;

        [[nodiscard]] TExpected<void, RHI::FRHIError> WaitIdle () noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIQueue>, RHI::FRHIError> GetQueue ( RHI::ERHIQueueType InType,
                                                                                       UInt32 InIndex ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>
        CreateBuffer ( const RHI::FRHIBufferDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHITexture>, RHI::FRHIError>
        CreateTexture ( const RHI::FRHITextureDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHITextureView>, RHI::FRHIError>
        CreateTextureView ( const RHI::FRHITextureViewDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHISampler>, RHI::FRHIError>
        CreateSampler ( const RHI::FRHISamplerDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIHeap>, RHI::FRHIError>
        CreateHeap ( const RHI::FRHIHeapDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIBufferPool>, RHI::FRHIError> CreateBufferPool () noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIUploadAllocator>, RHI::FRHIError>
        CreateUploadAllocator ( UInt64 InCapacity ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIReadbackAllocator>, RHI::FRHIError>
        CreateReadbackAllocator ( UInt64 InCapacity ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHITransientResourceAllocator>, RHI::FRHIError>
        CreateTransientResourceAllocator () noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIDefragmenter>, RHI::FRHIError> CreateDefragmenter () noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIBindingLayout>, RHI::FRHIError>
        CreateBindingLayout ( const RHI::FRHIBindingLayoutDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIBindingSet>, RHI::FRHIError>
        CreateBindingSet ( const TSharedRef<RHI::IRHIBindingLayout> &InLayout ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIDescriptorAllocator>, RHI::FRHIError>
        CreateDescriptorAllocator ( const RHI::FRHIDescriptorAllocatorDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIShader>, RHI::FRHIError>
        CreateShader ( const RHI::FRHIShaderDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIPipelineLayout>, RHI::FRHIError>
        CreatePipelineLayout ( const RHI::FRHIPipelineLayoutDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIPipeline>, RHI::FRHIError>
        CreateGraphicsPipeline ( const RHI::FRHIGraphicsPipelineDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIPipeline>, RHI::FRHIError>
        CreateComputePipeline ( const RHI::FRHIComputePipelineDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIPipelineCache>, RHI::FRHIError>
        CreatePipelineCache ( const RHI::FRHIPipelineCacheDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHICommandAllocator>, RHI::FRHIError>
        CreateCommandAllocator ( RHI::ERHIQueueType InQueueType ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHICommandList>, RHI::FRHIError>
        CreateCommandList ( const TSharedRef<RHI::IRHICommandAllocator> &InAllocator ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHISemaphore>, RHI::FRHIError>
        CreateSemaphore ( const RHI::FRHISemaphoreDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHISwapchain>, RHI::FRHIError>
        CreateSwapchain ( const RHI::FRHISwapchainDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIQueryPool>, RHI::FRHIError>
        CreateQueryPool ( const RHI::FRHIQueryPoolDesc &InDesc ) noexcept override;

        explicit FVulkanDevice ( TSharedRef<FVulkanDeviceContext> InContext ) noexcept;

    private:

        struct FQueueEntry
        {
            RHI::ERHIQueueType Type = RHI::ERHIQueueType::Graphics;
            UInt32 Index            = 0U;
            TSharedRef<FVulkanQueue> Queue;
        };

        TSharedRef<FVulkanDeviceContext> Context;
        TVector<FQueueEntry> Queues;
    };

} // namespace RHIVulkan

} // namespace Lumen
