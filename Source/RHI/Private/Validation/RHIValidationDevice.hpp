/**
 * @file RHIValidationDevice.hpp
 * @brief Declares the root validation device and wrapping factories.
 */
#pragma once

#include "RHIValidationBindings.hpp"
#include "RHIValidationCommandAllocator.hpp"
#include "RHIValidationCommandList.hpp"
#include "RHIValidationQueue.hpp"
#include "RHIValidationResources.hpp"
#include "RHIValidationSwapchain.hpp"
#include "RHIValidationTransientResourceAllocator.hpp"
#include <Device/IRHIDevice.hpp>

namespace Lumen
{

/**
 * @class FRHIValidationDevice
 * @brief Wraps a device and every child belonging to a validated domain.
 */
class RHI_EXPORT FRHIValidationDevice final : public IRHIDevice, public IRHIValidationObject
{
public:

    FRHIValidationDevice ( TSharedRef<IRHIDevice> InUnderlying, TSharedPtr<FRHIValidationState> InState ) noexcept;
    ~FRHIValidationDevice () noexcept override;
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    [[nodiscard]] FString GetDebugName () const noexcept override;
    void SetDebugName ( FStringView InName ) noexcept override;
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
    [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
    [[nodiscard]] const FRHICapabilities &GetCapabilities () const noexcept override;
    [[nodiscard]] const FRHILimits &GetLimits () const noexcept override;
    [[nodiscard]] ERHIDeviceStatus GetStatus () const noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> WaitIdle () noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIQueue>, FRHIError> GetQueue ( ERHIQueueType InType, UInt32 InIndex ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIBuffer>, FRHIError> CreateBuffer ( const FRHIBufferDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHITexture>, FRHIError> CreateTexture ( const FRHITextureDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHITextureView>, FRHIError>
    CreateTextureView ( const FRHITextureViewDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHISampler>, FRHIError> CreateSampler ( const FRHISamplerDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIHeap>, FRHIError> CreateHeap ( const FRHIHeapDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIBufferPool>, FRHIError> CreateBufferPool () noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIUploadAllocator>, FRHIError>
    CreateUploadAllocator ( UInt64 InCapacity ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIReadbackAllocator>, FRHIError>
    CreateReadbackAllocator ( UInt64 InCapacity ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHITransientResourceAllocator>, FRHIError>
    CreateTransientResourceAllocator () noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIDefragmenter>, FRHIError> CreateDefragmenter () noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIBindingLayout>, FRHIError>
    CreateBindingLayout ( const FRHIBindingLayoutDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIBindingSet>, FRHIError>
    CreateBindingSet ( const TSharedRef<IRHIBindingLayout> &InLayout ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIDescriptorAllocator>, FRHIError>
    CreateDescriptorAllocator ( const FRHIDescriptorAllocatorDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIShader>, FRHIError> CreateShader ( const FRHIShaderDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIPipelineLayout>, FRHIError>
    CreatePipelineLayout ( const FRHIPipelineLayoutDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIPipeline>, FRHIError>
    CreateGraphicsPipeline ( const FRHIGraphicsPipelineDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIPipeline>, FRHIError>
    CreateComputePipeline ( const FRHIComputePipelineDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIPipelineCache>, FRHIError>
    CreatePipelineCache ( const FRHIPipelineCacheDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHICommandAllocator>, FRHIError>
    CreateCommandAllocator ( ERHIQueueType InQueueType ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHICommandList>, FRHIError>
    CreateCommandList ( const TSharedRef<IRHICommandAllocator> &InAllocator ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHISemaphore>, FRHIError>
    CreateSemaphore ( const FRHISemaphoreDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHISwapchain>, FRHIError>
    CreateSwapchain ( const FRHISwapchainDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHIQueryPool>, FRHIError>
    CreateQueryPool ( const FRHIQueryPoolDesc &InDesc ) noexcept override;

    /** @return Shared white-box state used by validation tests. */
    [[nodiscard]] const TSharedPtr<FRHIValidationState> &GetState () const noexcept;

private:

    TSharedRef<IRHIDevice> Underlying;
    TSharedPtr<FRHIValidationState> State;
    TVector<TSharedRef<IRHIQueue>> Queues;
    HAL::FPlatformRecursiveSharedMutex QueueMutex;
};

} // namespace Lumen
