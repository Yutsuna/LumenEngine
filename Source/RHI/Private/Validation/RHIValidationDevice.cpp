/**
 * @file RHIValidationDevice.cpp
 * @brief Implements root device forwarding and child decoration.
 */
#include "RHIValidationDevice.hpp"
#include <HAL/LockGuard.hpp>
#include <Pipelines/RHIPipelineLayoutDesc.hpp>
#include <Validation/RHIValidation.hpp>

Lumen::FRHIValidationDevice::FRHIValidationDevice ( TSharedRef<IRHIDevice> InUnderlying,
                                                    TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
}

Lumen::FRHIValidationDevice::~FRHIValidationDevice () noexcept
{
    Queues.clear();
    State->ReportLeaks();
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationDevice )

const Lumen::FRHICapabilities &Lumen::FRHIValidationDevice::GetCapabilities () const noexcept
{
    return Underlying->GetCapabilities();
}

const Lumen::FRHILimits &Lumen::FRHIValidationDevice::GetLimits () const noexcept
{
    return Underlying->GetLimits();
}

Lumen::ERHIDeviceStatus Lumen::FRHIValidationDevice::GetStatus () const noexcept
{
    return Underlying->GetStatus();
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationDevice::WaitIdle () noexcept
{
    return Underlying->WaitIdle();
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIQueue>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::GetQueue ( const ERHIQueueType InType, const UInt32 InIndex ) noexcept
{
    auto Result = Underlying->GetQueue( InType, InIndex );
    if ( not Result )
    {
        return Result;
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( QueueMutex );
    IRHIObject *BackendQueue = UnwrapRHIObject( Result->Get() );
    for ( const auto &Queue : Queues )
    {
        if ( UnwrapRHIObject( Queue.Get() ) == BackendQueue )
        {
            return Queue;
        }
    }
    TSharedRef<IRHIQueue> Wrapped = MakeShared<FRHIValidationQueue>( *Result, State );
    Queues.push_back( Wrapped );
    return Wrapped;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBuffer>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateBuffer ( const FRHIBufferDesc &InDesc ) noexcept
{
    auto Result = Underlying->CreateBuffer( InDesc );
    if ( not Result )
    {
        return Result;
    }
    return MakeShared<FRHIValidationBuffer>( *Result, State );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHITexture>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateTexture ( const FRHITextureDesc &InDesc ) noexcept
{
    auto Result = Underlying->CreateTexture( InDesc );
    if ( not Result )
    {
        return Result;
    }
    return MakeShared<FRHIValidationTexture>( *Result, State );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHITextureView>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateTextureView ( const FRHITextureViewDesc &InDesc ) noexcept
{
    if ( not InDesc.Texture )
    {
        return TUnexpected<FRHIError>( State->GetReporter()
                                           ->Reject( ERHIValidationMessageCode::InvalidDescriptor, ERHIErrorCondition::Resource,
                                                     "Texture view has no parent", this )
                                           .Error() );
    }
    auto Owned = State->ValidateObject( *InDesc.Texture, ERHIValidationObjectKind::Texture );
    if ( not Owned )
    {
        return TUnexpected<FRHIError>( Owned.Error() );
    }
    FRHITextureViewDesc Unwrapped = InDesc;
    Unwrapped.Texture             = UnwrapRHIShared( InDesc.Texture );
    auto Result                   = Underlying->CreateTextureView( Unwrapped );
    if ( not Result )
    {
        return Result;
    }
    return MakeShared<FRHIValidationTextureView>( *Result, InDesc.Texture, State );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHISampler>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateSampler ( const FRHISamplerDesc &InDesc ) noexcept
{
    auto Result = Underlying->CreateSampler( InDesc );
    if ( not Result )
    {
        return Result;
    }
    return MakeShared<FRHIValidationSampler>( *Result, State );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIHeap>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateHeap ( const FRHIHeapDesc &InDesc ) noexcept
{
    return Underlying->CreateHeap( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBufferPool>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateBufferPool () noexcept
{
    return Underlying->CreateBufferPool();
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIUploadAllocator>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateUploadAllocator ( const UInt64 InCapacity ) noexcept
{
    return Underlying->CreateUploadAllocator( InCapacity );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIReadbackAllocator>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateReadbackAllocator ( const UInt64 InCapacity ) noexcept
{
    return Underlying->CreateReadbackAllocator( InCapacity );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHITransientResourceAllocator>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateTransientResourceAllocator () noexcept
{
    auto Result = Underlying->CreateTransientResourceAllocator();
    if ( not Result )
    {
        return Result;
    }
    return MakeShared<FRHIValidationTransientResourceAllocator>( *Result, State );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIDefragmenter>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateDefragmenter () noexcept
{
    return Underlying->CreateDefragmenter();
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBindingLayout>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateBindingLayout ( const FRHIBindingLayoutDesc &InDesc ) noexcept
{
    auto Result = Underlying->CreateBindingLayout( InDesc );
    if ( not Result )
    {
        return Result;
    }
    return MakeShared<FRHIValidationBindingLayout>( *Result, State );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBindingSet>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateBindingSet ( const TSharedRef<IRHIBindingLayout> &InLayout ) noexcept
{
    auto Owned = State->ValidateObject( *InLayout, ERHIValidationObjectKind::BindingLayout );
    if ( not Owned )
    {
        return TUnexpected<FRHIError>( Owned.Error() );
    }
    auto Result = Underlying->CreateBindingSet( UnwrapRHIShared( InLayout ) );
    if ( not Result )
    {
        return Result;
    }
    return MakeShared<FRHIValidationBindingSet>( *Result, InLayout, State );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIDescriptorAllocator>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateDescriptorAllocator ( const FRHIDescriptorAllocatorDesc &InDesc ) noexcept
{
    return Underlying->CreateDescriptorAllocator( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIShader>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateShader ( const FRHIShaderDesc &InDesc ) noexcept
{
    return Underlying->CreateShader( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIPipelineLayout>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreatePipelineLayout ( const FRHIPipelineLayoutDesc &InDesc ) noexcept
{
    FRHIPipelineLayoutDesc Unwrapped;
    Unwrapped.InlineConstants = InDesc.InlineConstants;
    Unwrapped.BindingLayouts.reserve( InDesc.BindingLayouts.size() );
    for ( const auto &Layout : InDesc.BindingLayouts )
    {
        auto Owned = State->ValidateObject( *Layout, ERHIValidationObjectKind::BindingLayout );
        if ( not Owned )
        {
            return TUnexpected<FRHIError>( Owned.Error() );
        }
        Unwrapped.BindingLayouts.push_back( UnwrapRHIShared( Layout ) );
    }
    return Underlying->CreatePipelineLayout( Unwrapped );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIPipeline>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateGraphicsPipeline ( const FRHIGraphicsPipelineDesc &InDesc ) noexcept
{
    return Underlying->CreateGraphicsPipeline( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIPipeline>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateComputePipeline ( const FRHIComputePipelineDesc &InDesc ) noexcept
{
    return Underlying->CreateComputePipeline( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIPipelineCache>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreatePipelineCache ( const FRHIPipelineCacheDesc &InDesc ) noexcept
{
    return Underlying->CreatePipelineCache( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHICommandAllocator>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateCommandAllocator ( const ERHIQueueType InQueueType ) noexcept
{
    auto Result = Underlying->CreateCommandAllocator( InQueueType );
    if ( not Result )
    {
        return Result;
    }
    return MakeShared<FRHIValidationCommandAllocator>( *Result, State );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHICommandList>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateCommandList ( const TSharedRef<IRHICommandAllocator> &InAllocator ) noexcept
{
    auto Owned = State->ValidateObject( *InAllocator, ERHIValidationObjectKind::CommandAllocator );
    if ( not Owned )
    {
        return TUnexpected<FRHIError>( Owned.Error() );
    }
    auto *ValidationAllocator = dynamic_cast<FRHIValidationCommandAllocator *>( InAllocator.Get() );
    if ( ValidationAllocator == nullptr )
    {
        return TUnexpected<FRHIError>( State->GetReporter()
                                           ->Reject( ERHIValidationMessageCode::ForeignObject, ERHIErrorCondition::Command,
                                                     "Command allocator belongs to another validation layer", this )
                                           .Error() );
    }
    auto Result = Underlying->CreateCommandList( UnwrapRHIShared( InAllocator ) );
    if ( not Result )
    {
        return Result;
    }
    const TSharedRef<FRHIValidationCommandAllocator> Allocator =
        StaticCastSharedRef<FRHIValidationCommandAllocator>( InAllocator );
    return MakeShared<FRHIValidationCommandList>( *Result, Allocator, State );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHISemaphore>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateSemaphore ( const FRHISemaphoreDesc &InDesc ) noexcept
{
    return Underlying->CreateSemaphore( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHISwapchain>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateSwapchain ( const FRHISwapchainDesc &InDesc ) noexcept
{
    auto Result = Underlying->CreateSwapchain( InDesc );
    if ( not Result )
    {
        return Result;
    }
    return MakeShared<FRHIValidationSwapchain>( *Result, State );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIQueryPool>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateQueryPool ( const FRHIQueryPoolDesc &InDesc ) noexcept
{
    return Underlying->CreateQueryPool( InDesc );
}

const Lumen::TSharedPtr<Lumen::FRHIValidationState> &Lumen::FRHIValidationDevice::GetState () const noexcept
{
    return State;
}
