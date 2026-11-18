/**
 * @file IRHIDevice.hpp
 * @brief Declares the backend-independent factory for all RHI objects.
 */

#pragma once

#include <RHIExport.hpp>

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
 * @details Every factory returns an object owned by the caller and is
 *          thread-safe unless a concrete descriptor documents external
 *          ownership. Factories reject requests before native allocation when
 *          a capability is missing or a limit is exceeded, and fail with
 *          DeviceLost or DeviceClosed after loss or closure. No method throws;
 *          all failures are returned as FRHIError values.
 */
class RHI_EXPORT IRHIDevice : public IRHIObject
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

    /**
     * @brief Creates a buffer owned by the caller.
     * @param InDesc Immutable buffer description.
     * @return The created buffer, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIBuffer>, FRHIError> CreateBuffer ( const FRHIBufferDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a texture owned by the caller.
     * @param InDesc Immutable texture description.
     * @return The created texture, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHITexture>, FRHIError>
    CreateTexture ( const FRHITextureDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a texture view owned by the caller.
     * @param InDesc Immutable view description.
     * @return The created texture view, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHITextureView>, FRHIError>
    CreateTextureView ( const FRHITextureViewDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a sampler owned by the caller.
     * @param InDesc Immutable sampler description.
     * @return The created sampler, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHISampler>, FRHIError>
    CreateSampler ( const FRHISamplerDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a heap owned by the caller.
     * @param InDesc Heap size, memory class and alignment requirements.
     * @return The created heap, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIHeap>, FRHIError> CreateHeap ( const FRHIHeapDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a buffer pool owned by the caller.
     * @return The created buffer pool, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIBufferPool>, FRHIError> CreateBufferPool () noexcept = 0;

    /**
     * @brief Creates a upload allocator owned by the caller.
     * @param InCapacity Byte capacity of the allocator.
     * @return The created upload allocator, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIUploadAllocator>, FRHIError>
    CreateUploadAllocator ( UInt64 InCapacity ) noexcept = 0;

    /**
     * @brief Creates a readback allocator owned by the caller.
     * @param InCapacity Byte capacity of the allocator.
     * @return The created readback allocator, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIReadbackAllocator>, FRHIError>
    CreateReadbackAllocator ( UInt64 InCapacity ) noexcept = 0;

    /**
     * @brief Creates a transient resource allocator owned by the caller.
     * @return The created transient resource allocator, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHITransientResourceAllocator>, FRHIError>
    CreateTransientResourceAllocator () noexcept = 0;

    /**
     * @brief Creates a defragmenter owned by the caller.
     * @return The created defragmenter, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIDefragmenter>, FRHIError> CreateDefragmenter () noexcept = 0;

    /**
     * @brief Creates a binding layout owned by the caller.
     * @param InDesc Immutable slot layout description.
     * @return The created binding layout, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIBindingLayout>, FRHIError>
    CreateBindingLayout ( const FRHIBindingLayoutDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a binding set owned by the caller.
     * @param InLayout Immutable layout the set conforms to.
     * @return The created binding set, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIBindingSet>, FRHIError>
    CreateBindingSet ( const TSharedRef<IRHIBindingLayout> &InLayout ) noexcept = 0;

    /**
     * @brief Creates a descriptor allocator owned by the caller.
     * @param InDesc Validated descriptor allocator policy.
     * @return The created descriptor allocator, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIDescriptorAllocator>, FRHIError>
    CreateDescriptorAllocator ( const FRHIDescriptorAllocatorDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a shader owned by the caller.
     * @param InDesc Shader bytecode, format and reflection description.
     * @return The created shader, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIShader>, FRHIError> CreateShader ( const FRHIShaderDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a pipeline layout owned by the caller.
     * @param InDesc Binding layouts and inline constant ranges.
     * @return The created pipeline layout, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIPipelineLayout>, FRHIError>
    CreatePipelineLayout ( const FRHIPipelineLayoutDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a graphics pipeline owned by the caller.
     * @param InDesc Complete graphics pipeline description.
     * @return The created graphics pipeline, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIPipeline>, FRHIError>
    CreateGraphicsPipeline ( const FRHIGraphicsPipelineDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a compute pipeline owned by the caller.
     * @param InDesc Complete compute pipeline description.
     * @return The created compute pipeline, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIPipeline>, FRHIError>
    CreateComputePipeline ( const FRHIComputePipelineDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a pipeline cache owned by the caller.
     * @param InDesc Cache version and limits.
     * @return The created pipeline cache, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIPipelineCache>, FRHIError>
    CreatePipelineCache ( const FRHIPipelineCacheDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a command allocator owned by the caller.
     * @param InQueueType Queue family whose lists this allocator recycles.
     * @return The created command allocator, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHICommandAllocator>, FRHIError>
    CreateCommandAllocator ( ERHIQueueType InQueueType ) noexcept = 0;

    /**
     * @brief Creates a command list owned by the caller.
     * @param InAllocator Allocator that owns and recycles the list.
     * @return The created command list, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHICommandList>, FRHIError>
    CreateCommandList ( const TSharedRef<IRHICommandAllocator> &InAllocator ) noexcept = 0;

    /**
     * @brief Creates a semaphore owned by the caller.
     * @param InDesc Semaphore type and initial value.
     * @return The created semaphore, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHISemaphore>, FRHIError>
    CreateSemaphore ( const FRHISemaphoreDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a swapchain owned by the caller.
     * @param InDesc Surface payload and presentation requirements.
     * @return The created swapchain, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHISwapchain>, FRHIError>
    CreateSwapchain ( const FRHISwapchainDesc &InDesc ) noexcept = 0;

    /**
     * @brief Creates a query pool owned by the caller.
     * @param InDesc Query type and count.
     * @return The created query pool, or a structured FRHIError as described on the class.
     */
    [[nodiscard]] virtual TExpected<TSharedRef<IRHIQueryPool>, FRHIError>
    CreateQueryPool ( const FRHIQueryPoolDesc &InDesc ) noexcept = 0;
};

} // namespace Lumen
