/**
 * @file OpenGLDevice.hpp
 * @brief Declares the OpenGL IRHIDevice implementation built on FRHIDeviceBase.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Device/RHIDeviceBase.hpp>
#include <RHI/Module/RHIAdapterInfo.hpp>
#include <RHI/Module/RHIDeviceDesc.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLDevice
     * @brief OpenGL device: identity, status, negotiated factories and logical queues.
     */
    class FOpenGLDevice final : public RHI::FRHIDeviceBase, public FNonCopyable, public FNonMovable
    {
    public:

        /** @brief Negotiates the adapter contract, creates the GL executor and queues. */
        [[nodiscard]] static TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError>
        Create ( const RHI::FRHIAdapterInfo &InAdapter, const RHI::FRHIDeviceDesc &InDesc ) noexcept;

        /** @brief Constructs the device from shared execution state. */
        FOpenGLDevice ( TSharedRef<FOpenGLDeviceContext> InContext ) noexcept;

        ~FOpenGLDevice () noexcept override;

        /** @brief Marks the device lost for conformance error-path scenarios. */
        void InjectDeviceLost () noexcept;

        [[nodiscard]] RHI::ERHIDeviceStatus GetStatus () const noexcept override;

        [[nodiscard]] RHI::FRHIExpected WaitIdle () noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIQueue>, RHI::FRHIError> GetQueue ( RHI::ERHIQueueType InType,
                                                                                       UInt32 InIndex ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>
        CreateBuffer ( const RHI::FRHIBufferDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>
        CreatePlacedBuffer ( const RHI::FRHIBufferDesc &InDesc,
                             TSharedRef<RHI::IRHIHeap> InHeap,
                             const RHI::FRHIAllocation &InAllocation ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHITexture>, RHI::FRHIError>
        CreateTexture ( const RHI::FRHITextureDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHITexture>, RHI::FRHIError>
        CreatePlacedTexture ( const RHI::FRHITextureDesc &InDesc,
                              TSharedRef<RHI::IRHIHeap> InHeap,
                              const RHI::FRHIAllocation &InAllocation ) noexcept override;

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

    private:

        /** @brief Creates and registers one queue; out-of-line to avoid -O3 false positives. */
        LUMEN_NO_INLINE void AddQueue ( RHI::ERHIQueueType InType, UInt32 InIndex ) noexcept;

        struct FQueueEntry
        {
            RHI::ERHIQueueType Type = RHI::ERHIQueueType::Graphics;
            UInt32 Index            = 0U;
            TSharedRef<RHI::IRHIQueue> Queue;
        };

        TSharedRef<FOpenGLDeviceContext> Context;
        TVector<FQueueEntry> Queues;
    };

} // namespace RHIOpenGL

} // namespace Lumen
