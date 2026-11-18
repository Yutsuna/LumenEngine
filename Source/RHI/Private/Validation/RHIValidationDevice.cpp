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
    return Underlying->GetQueue( InType, InIndex )
        .AndThen( [this] ( TSharedRef<IRHIQueue> &&InQueue ) noexcept -> TExpected<TSharedRef<IRHIQueue>, FRHIError>
    {
        HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( QueueMutex );
        IRHIObject *BackendQueue = UnwrapRHIObject( InQueue.Get() );
        for ( const TSharedRef<IRHIQueue> &Queue : Queues )
        {
            if ( UnwrapRHIObject( Queue.Get() ) == BackendQueue )
            {
                return TExpected<TSharedRef<IRHIQueue>, FRHIError>( Queue );
            }
        }
        const TSharedRef<IRHIQueue> Wrapped = MakeShared<FRHIValidationQueue>( InQueue, State );
        Queues.push_back( Wrapped );
        return TExpected<TSharedRef<IRHIQueue>, FRHIError>( Wrapped );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBuffer>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateBuffer ( const FRHIBufferDesc &InDesc ) noexcept
{
    return Underlying->CreateBuffer( InDesc ).Transform(
        [this] ( TSharedRef<IRHIBuffer> &&InBuffer ) noexcept -> TSharedRef<IRHIBuffer>
    {
        return MakeShared<FRHIValidationBuffer>( std::move( InBuffer ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHITexture>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateTexture ( const FRHITextureDesc &InDesc ) noexcept
{
    return Underlying->CreateTexture( InDesc ).Transform(
        [this] ( TSharedRef<IRHITexture> &&InTexture ) noexcept -> TSharedRef<IRHITexture>
    {
        return MakeShared<FRHIValidationTexture>( std::move( InTexture ), State );
    } );
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
    return State->ValidateObject( *InDesc.Texture, ERHIValidationObjectKind::Texture )
        .AndThen( [this, &InDesc] () noexcept -> TExpected<TSharedRef<IRHITextureView>, FRHIError>
    {
        FRHITextureViewDesc Unwrapped = InDesc;
        Unwrapped.Texture             = UnwrapRHIShared( InDesc.Texture );
        return Underlying->CreateTextureView( Unwrapped )
            .Transform( [this, &InDesc] ( TSharedRef<IRHITextureView> &&InView ) noexcept -> TSharedRef<IRHITextureView>
        {
            return MakeShared<FRHIValidationTextureView>( std::move( InView ), InDesc.Texture, State );
        } );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHISampler>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateSampler ( const FRHISamplerDesc &InDesc ) noexcept
{
    return Underlying->CreateSampler( InDesc ).Transform(
        [this] ( TSharedRef<IRHISampler> &&InSampler ) noexcept -> TSharedRef<IRHISampler>
    {
        return MakeShared<FRHIValidationSampler>( std::move( InSampler ), State );
    } );
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
    return Underlying->CreateTransientResourceAllocator().Transform(
        [this] ( TSharedRef<IRHITransientResourceAllocator> &&InAllocator ) noexcept -> TSharedRef<IRHITransientResourceAllocator>
    {
        return MakeShared<FRHIValidationTransientResourceAllocator>( std::move( InAllocator ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIDefragmenter>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateDefragmenter () noexcept
{
    return Underlying->CreateDefragmenter().Transform(
        [this] ( TSharedRef<IRHIDefragmenter> &&InDefragmenter ) noexcept -> TSharedRef<IRHIDefragmenter>
    {
        return MakeShared<FRHIValidationDefragmenter>( std::move( InDefragmenter ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBindingLayout>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateBindingLayout ( const FRHIBindingLayoutDesc &InDesc ) noexcept
{
    return Underlying->CreateBindingLayout( InDesc ).Transform(
        [this] ( TSharedRef<IRHIBindingLayout> &&InLayout ) noexcept -> TSharedRef<IRHIBindingLayout>
    {
        return MakeShared<FRHIValidationBindingLayout>( std::move( InLayout ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBindingSet>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateBindingSet ( const TSharedRef<IRHIBindingLayout> &InLayout ) noexcept
{
    return State->ValidateObject( *InLayout, ERHIValidationObjectKind::BindingLayout )
        .AndThen( [this, &InLayout] () noexcept -> TExpected<TSharedRef<IRHIBindingSet>, FRHIError>
    {
        return Underlying->CreateBindingSet( UnwrapRHIShared( InLayout ) )
            .Transform( [this, &InLayout] ( TSharedRef<IRHIBindingSet> &&InSet ) noexcept -> TSharedRef<IRHIBindingSet>
        {
            return MakeShared<FRHIValidationBindingSet>( std::move( InSet ), InLayout, State );
        } );
    } );
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
    for ( const TSharedRef<IRHIBindingLayout> &Layout : InDesc.BindingLayouts )
    {
        const TExpected<void, FRHIError> Owned = State->ValidateObject( *Layout, ERHIValidationObjectKind::BindingLayout );
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
    return Underlying->CreateCommandAllocator( InQueueType )
        .Transform( [this] ( TSharedRef<IRHICommandAllocator> &&InAllocator ) noexcept -> TSharedRef<IRHICommandAllocator>
    {
        return MakeShared<FRHIValidationCommandAllocator>( std::move( InAllocator ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHICommandList>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateCommandList ( const TSharedRef<IRHICommandAllocator> &InAllocator ) noexcept
{
    return State->ValidateObject( *InAllocator, ERHIValidationObjectKind::CommandAllocator )
        .AndThen( [this, &InAllocator] () noexcept -> TExpected<TSharedRef<IRHICommandList>, FRHIError>
    {
        auto *ValidationAllocator = dynamic_cast<FRHIValidationCommandAllocator *>( InAllocator.Get() );
        if ( ValidationAllocator == nullptr )
        {
            return TUnexpected<FRHIError>( State->GetReporter()
                                               ->Reject( ERHIValidationMessageCode::ForeignObject, ERHIErrorCondition::Command,
                                                         "Command allocator belongs to another validation layer", this )
                                               .Error() );
        }
        return Underlying->CreateCommandList( UnwrapRHIShared( InAllocator ) )
            .Transform( [this, &InAllocator] ( TSharedRef<IRHICommandList> &&InList ) noexcept -> TSharedRef<IRHICommandList>
        {
            const TSharedRef<FRHIValidationCommandAllocator> Allocator =
                StaticCastSharedRef<FRHIValidationCommandAllocator>( InAllocator );
            return MakeShared<FRHIValidationCommandList>( std::move( InList ), Allocator, State );
        } );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHISemaphore>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateSemaphore ( const FRHISemaphoreDesc &InDesc ) noexcept
{
    return Underlying->CreateSemaphore( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHISwapchain>, Lumen::FRHIError>
Lumen::FRHIValidationDevice::CreateSwapchain ( const FRHISwapchainDesc &InDesc ) noexcept
{
    return Underlying->CreateSwapchain( InDesc ).Transform(
        [this] ( TSharedRef<IRHISwapchain> &&InSwapchain ) noexcept -> TSharedRef<IRHISwapchain>
    {
        return MakeShared<FRHIValidationSwapchain>( std::move( InSwapchain ), State );
    } );
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
