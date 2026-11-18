/**
 * @file NullDevice.hpp
 * @brief Declares the complete deterministic Null device.
 */

#pragma once

#include <Device/NullDeviceDesc.hpp>
#include <Memory/UniquePtr.hpp>
#include <RHI/Device/RHIDeviceBase.hpp>

namespace Lumen
{

namespace RHI
{

    namespace Tests
    {

        class FNullDescriptorPoolBackend;
        class FNullQueue;

        /**
         * @class FNullDevice
         * @brief Reference implementation of every IRHIDevice factory.
         */
        class FNullDevice final : public FRHIDeviceBase
        {
        public:

            /** @brief Creates a ready device with the supplied capabilities and fault plan. */
            explicit FNullDevice ( FNullDeviceDesc InDesc = {} ) noexcept;
            ~FNullDevice () noexcept override;

            TExpected<void, FRHIError> WaitIdle () noexcept override;
            TExpected<TSharedRef<IRHIQueue>, FRHIError> GetQueue ( ERHIQueueType InType, UInt32 InIndex ) noexcept override;
            TExpected<TSharedRef<IRHIBuffer>, FRHIError> CreateBuffer ( const FRHIBufferDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHITexture>, FRHIError> CreateTexture ( const FRHITextureDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHITextureView>, FRHIError>
            CreateTextureView ( const FRHITextureViewDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHISampler>, FRHIError> CreateSampler ( const FRHISamplerDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHIHeap>, FRHIError> CreateHeap ( const FRHIHeapDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHIBufferPool>, FRHIError> CreateBufferPool () noexcept override;
            TExpected<TSharedRef<IRHIUploadAllocator>, FRHIError> CreateUploadAllocator ( UInt64 InCapacity ) noexcept override;
            TExpected<TSharedRef<IRHIReadbackAllocator>, FRHIError>
            CreateReadbackAllocator ( UInt64 InCapacity ) noexcept override;
            TExpected<TSharedRef<IRHITransientResourceAllocator>, FRHIError>
            CreateTransientResourceAllocator () noexcept override;
            TExpected<TSharedRef<IRHIDefragmenter>, FRHIError> CreateDefragmenter () noexcept override;
            TExpected<TSharedRef<IRHIBindingLayout>, FRHIError>
            CreateBindingLayout ( const FRHIBindingLayoutDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHIBindingSet>, FRHIError>
            CreateBindingSet ( const TSharedRef<IRHIBindingLayout> &InLayout ) noexcept override;
            TExpected<TSharedRef<IRHIDescriptorAllocator>, FRHIError>
            CreateDescriptorAllocator ( const FRHIDescriptorAllocatorDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHIShader>, FRHIError> CreateShader ( const FRHIShaderDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHIPipelineLayout>, FRHIError>
            CreatePipelineLayout ( const FRHIPipelineLayoutDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHIPipeline>, FRHIError>
            CreateGraphicsPipeline ( const FRHIGraphicsPipelineDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHIPipeline>, FRHIError>
            CreateComputePipeline ( const FRHIComputePipelineDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHIPipelineCache>, FRHIError>
            CreatePipelineCache ( const FRHIPipelineCacheDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHICommandAllocator>, FRHIError>
            CreateCommandAllocator ( ERHIQueueType InQueueType ) noexcept override;
            TExpected<TSharedRef<IRHICommandList>, FRHIError>
            CreateCommandList ( const TSharedRef<IRHICommandAllocator> &InAllocator ) noexcept override;
            TExpected<TSharedRef<IRHISemaphore>, FRHIError> CreateSemaphore ( const FRHISemaphoreDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHISwapchain>, FRHIError> CreateSwapchain ( const FRHISwapchainDesc &InDesc ) noexcept override;
            TExpected<TSharedRef<IRHIQueryPool>, FRHIError> CreateQueryPool ( const FRHIQueryPoolDesc &InDesc ) noexcept override;

            /** @brief Marks the device lost and causes subsequent work to fail. */
            void MarkLost () noexcept;
            /** @return Shared fault plan for test-specific failures. */
            [[nodiscard]] TSharedPtr<FNullFaultPlan> GetFaultPlan () const noexcept;

        private:

            [[nodiscard]] TOptional<FRHIError> CheckOperation ( FStringView InOperation ) noexcept;
            template <typename ObjectType>
            [[nodiscard]] TExpected<TSharedRef<ObjectType>, FRHIError> Unsupported ( FStringView InOperation ) noexcept;

        private:

            FNullDeviceDesc Desc;
            TUniquePtr<FNullDescriptorPoolBackend> DescriptorBackend;
            TVector<TSharedRef<FNullQueue>> Queues;
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen

#include "Inline/NullDevice.inl"
