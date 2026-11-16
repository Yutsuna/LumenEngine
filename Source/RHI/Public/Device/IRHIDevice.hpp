/**
 * @file IRHIDevice.hpp
 * @brief Declares the backend-independent factory for all RHI objects.
 */
#pragma once

#include <Device/RHIDeviceStatus.hpp>
#include <Memory/SharedPtr.hpp>
#include <Templates/Expected.hpp>
#include <Types/IRHIObject.hpp>
#include <Types/RHICapabilities.hpp>
#include <Types/RHILimits.hpp>
#include <Types/RHIQueueType.hpp>

namespace Lumen
{

class IRHIQueue;
class IRHIBuffer;
class IRHITexture;
class IRHITextureView;
class IRHISampler;
class IRHIHeap;
class IRHIBufferPool;
class IRHIUploadAllocator;
class IRHIReadbackAllocator;
class IRHITransientResourceAllocator;
class IRHIDefragmenter;
class IRHIBindingLayout;
class IRHIBindingSet;
class IRHIDescriptorAllocator;
class IRHIShader;
class IRHIPipelineLayout;
class IRHIPipeline;
class IRHIPipelineCache;
class IRHICommandAllocator;
class IRHICommandList;
class IRHISemaphore;
class IRHISwapchain;
class IRHIQueryPool;
struct FRHIBufferDesc;
struct FRHITextureDesc;
struct FRHITextureViewDesc;
struct FRHISamplerDesc;
struct FRHIHeapDesc;
struct FRHIBindingLayoutDesc;
struct FRHIDescriptorAllocatorDesc;
struct FRHIShaderDesc;
struct FRHIPipelineLayoutDesc;
struct FRHIGraphicsPipelineDesc;
struct FRHIComputePipelineDesc;
struct FRHIPipelineCacheDesc;
struct FRHISemaphoreDesc;
struct FRHISwapchainDesc;
struct FRHIQueryPoolDesc;

/**
 * @class IRHIDevice
 * @brief Factory and operational status contract implemented by each backend.
 */
class IRHIDevice : public IRHIObject
{
public:

    /** @return Negotiated immutable capabilities. */
    [[nodiscard]] virtual const FRHICapabilities &GetCapabilities () const noexcept = 0;
    /** @return Negotiated immutable limits. */
    [[nodiscard]] virtual const FRHILimits &GetLimits () const noexcept = 0;
    /** @return Current ready/lost/closed state. */
    [[nodiscard]] virtual ERHIDeviceStatus GetStatus () const noexcept = 0;
    /** @brief Waits for all queues to become idle. */
    [[nodiscard]] virtual TExpected<void, FRHIError> WaitIdle () noexcept = 0;
    /** @brief Gets a previously requested queue. */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIQueue>, FRHIError> GetQueue ( ERHIQueueType InType,
                                                                                 UInt32 InIndex ) noexcept = 0;

    [[nodiscard]] virtual TExpected<TSharedRef<IRHIBuffer>, FRHIError> CreateBuffer ( const FRHIBufferDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHITexture>, FRHIError>
    CreateTexture ( const FRHITextureDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHITextureView>, FRHIError>
    CreateTextureView ( const FRHITextureViewDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHISampler>, FRHIError>
    CreateSampler ( const FRHISamplerDesc &InDesc ) noexcept                                                            = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIHeap>, FRHIError> CreateHeap ( const FRHIHeapDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIBufferPool>, FRHIError> CreateBufferPool () noexcept                 = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIUploadAllocator>, FRHIError>
    CreateUploadAllocator ( UInt64 InCapacity ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIReadbackAllocator>, FRHIError>
    CreateReadbackAllocator ( UInt64 InCapacity ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHITransientResourceAllocator>, FRHIError>
    CreateTransientResourceAllocator () noexcept                                                            = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIDefragmenter>, FRHIError> CreateDefragmenter () noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIBindingLayout>, FRHIError>
    CreateBindingLayout ( const FRHIBindingLayoutDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIBindingSet>, FRHIError>
    CreateBindingSet ( const TSharedRef<IRHIBindingLayout> &InLayout ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIDescriptorAllocator>, FRHIError>
    CreateDescriptorAllocator ( const FRHIDescriptorAllocatorDesc &InDesc ) noexcept                                          = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIShader>, FRHIError> CreateShader ( const FRHIShaderDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIPipelineLayout>, FRHIError>
    CreatePipelineLayout ( const FRHIPipelineLayoutDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIPipeline>, FRHIError>
    CreateGraphicsPipeline ( const FRHIGraphicsPipelineDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIPipeline>, FRHIError>
    CreateComputePipeline ( const FRHIComputePipelineDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIPipelineCache>, FRHIError>
    CreatePipelineCache ( const FRHIPipelineCacheDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHICommandAllocator>, FRHIError>
    CreateCommandAllocator ( ERHIQueueType InQueueType ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHICommandList>, FRHIError>
    CreateCommandList ( const TSharedRef<IRHICommandAllocator> &InAllocator ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHISemaphore>, FRHIError>
    CreateSemaphore ( const FRHISemaphoreDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHISwapchain>, FRHIError>
    CreateSwapchain ( const FRHISwapchainDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIQueryPool>, FRHIError>
    CreateQueryPool ( const FRHIQueryPoolDesc &InDesc ) noexcept = 0;
};

} // namespace Lumen
