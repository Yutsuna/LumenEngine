/**
 * @file RHIValidationDevice.cpp
 * @brief Implements root device forwarding and child decoration.
 */

#include "RHIValidationDevice.hpp"
#include "RHIValidationMemory.hpp"
#include <HAL/LockGuard.hpp>
#include <RHI/Memory/RHIAllocation.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <RHI/Validation/RHIValidation.hpp>

/**
 * Public
 */

Lumen::RHI::FRHIValidationDevice::FRHIValidationDevice ( TSharedRef<IRHIDevice> InUnderlying,
                                                         TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    /* */
}

Lumen::RHI::FRHIValidationDevice::~FRHIValidationDevice () noexcept
{
    Queues.clear();
    State->ReportLeaks();
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationDevice )

const Lumen::RHI::FRHICapabilities &Lumen::RHI::FRHIValidationDevice::GetCapabilities () const noexcept
{
    return Underlying->GetCapabilities();
}

const Lumen::RHI::FRHILimits &Lumen::RHI::FRHIValidationDevice::GetLimits () const noexcept
{
    return Underlying->GetLimits();
}

Lumen::RHI::ERHIDeviceStatus Lumen::RHI::FRHIValidationDevice::GetStatus () const noexcept
{
    return Underlying->GetStatus();
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationDevice::WaitIdle () noexcept
{
    return Underlying->WaitIdle();
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIQueue>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::GetQueue ( const ERHIQueueType InType, const UInt32 InIndex ) noexcept
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
                return { Queue };
            }
        }
        const TSharedRef<IRHIQueue> Wrapped = MakeShared<FRHIValidationQueue>( InQueue, State );
        Queues.push_back( Wrapped );
        return { Wrapped };
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateBuffer ( const FRHIBufferDesc &InDesc ) noexcept
{
    return Underlying->CreateBuffer( InDesc ).Transform(
        [this] ( TSharedRef<IRHIBuffer> &&InBuffer ) noexcept -> TSharedRef<IRHIBuffer>
    {
        return MakeShared<FRHIValidationBuffer>( std::move( InBuffer ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreatePlacedBuffer ( const FRHIBufferDesc &InDesc,
                                                       TSharedRef<IRHIHeap> InHeap,
                                                       const FRHIAllocation &InAllocation ) noexcept
{
    if ( InAllocation.Heap == nullptr or UnwrapRHIObject( InAllocation.Heap ) != UnwrapRHIObject( InHeap.Get() ) )
    {
        return TUnexpected<FRHIError>( State->GetReporter()
                                           ->Reject( ERHIValidationMessageCode::InvalidDescriptor, ERHIErrorCondition::Memory,
                                                     "Placed buffer allocation does not identify the supplied heap", this )
                                           .Error() );
    }
    return State->ValidateObject( *InHeap, ERHIValidationObjectKind::Heap )
        .AndThen( [this, &InDesc, &InHeap, &InAllocation] () noexcept -> TExpected<TSharedRef<IRHIBuffer>, FRHIError>
    {
        TSharedRef<IRHIHeap> UnwrappedHeap = UnwrapRHIShared( InHeap );
        FRHIAllocation UnwrappedAllocation = InAllocation;
        UnwrappedAllocation.Heap           = UnwrappedHeap.Get();
        return Underlying->CreatePlacedBuffer( InDesc, UnwrappedHeap, UnwrappedAllocation )
            .Transform( [this] ( TSharedRef<IRHIBuffer> &&InBuffer ) noexcept -> TSharedRef<IRHIBuffer>
        {
            return MakeShared<FRHIValidationBuffer>( std::move( InBuffer ), State );
        } );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateTexture ( const FRHITextureDesc &InDesc ) noexcept
{
    return Underlying->CreateTexture( InDesc ).Transform(
        [this] ( TSharedRef<IRHITexture> &&InTexture ) noexcept -> TSharedRef<IRHITexture>
    {
        return MakeShared<FRHIValidationTexture>( std::move( InTexture ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreatePlacedTexture ( const FRHITextureDesc &InDesc,
                                                        TSharedRef<IRHIHeap> InHeap,
                                                        const FRHIAllocation &InAllocation ) noexcept
{
    if ( InAllocation.Heap == nullptr or UnwrapRHIObject( InAllocation.Heap ) != UnwrapRHIObject( InHeap.Get() ) )
    {
        return TUnexpected<FRHIError>( State->GetReporter()
                                           ->Reject( ERHIValidationMessageCode::InvalidDescriptor, ERHIErrorCondition::Memory,
                                                     "Placed texture allocation does not identify the supplied heap", this )
                                           .Error() );
    }
    return State->ValidateObject( *InHeap, ERHIValidationObjectKind::Heap )
        .AndThen( [this, &InDesc, &InHeap, &InAllocation] () noexcept -> TExpected<TSharedRef<IRHITexture>, FRHIError>
    {
        TSharedRef<IRHIHeap> UnwrappedHeap = UnwrapRHIShared( InHeap );
        FRHIAllocation UnwrappedAllocation = InAllocation;
        UnwrappedAllocation.Heap           = UnwrappedHeap.Get();
        return Underlying->CreatePlacedTexture( InDesc, UnwrappedHeap, UnwrappedAllocation )
            .Transform( [this] ( TSharedRef<IRHITexture> &&InTexture ) noexcept -> TSharedRef<IRHITexture>
        {
            return MakeShared<FRHIValidationTexture>( std::move( InTexture ), State );
        } );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITextureView>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateTextureView ( const FRHITextureViewDesc &InDesc ) noexcept
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

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISampler>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateSampler ( const FRHISamplerDesc &InDesc ) noexcept
{
    return Underlying->CreateSampler( InDesc ).Transform(
        [this] ( TSharedRef<IRHISampler> &&InSampler ) noexcept -> TSharedRef<IRHISampler>
    {
        return MakeShared<FRHIValidationSampler>( std::move( InSampler ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIHeap>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateHeap ( const FRHIHeapDesc &InDesc ) noexcept
{
    return Underlying->CreateHeap( InDesc ).Transform( [this] ( TSharedRef<IRHIHeap> &&InHeap ) noexcept -> TSharedRef<IRHIHeap>
    {
        return MakeShared<FRHIValidationHeap>( std::move( InHeap ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBufferPool>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateBufferPool () noexcept
{
    return Underlying->CreateBufferPool();
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIUploadAllocator>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateUploadAllocator ( const UInt64 InCapacity ) noexcept
{
    return Underlying->CreateUploadAllocator( InCapacity )
        .Transform( [this] ( TSharedRef<IRHIUploadAllocator> &&InAllocator ) noexcept -> TSharedRef<IRHIUploadAllocator>
    {
        return MakeShared<FRHIValidationUploadAllocator>( std::move( InAllocator ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIReadbackAllocator>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateReadbackAllocator ( const UInt64 InCapacity ) noexcept
{
    return Underlying->CreateReadbackAllocator( InCapacity )
        .Transform( [this] ( TSharedRef<IRHIReadbackAllocator> &&InAllocator ) noexcept -> TSharedRef<IRHIReadbackAllocator>
    {
        return MakeShared<FRHIValidationReadbackAllocator>( std::move( InAllocator ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITransientResourceAllocator>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateTransientResourceAllocator () noexcept
{
    return Underlying->CreateTransientResourceAllocator().Transform(
        [this] ( TSharedRef<IRHITransientResourceAllocator> &&InAllocator ) noexcept -> TSharedRef<IRHITransientResourceAllocator>
    {
        return MakeShared<FRHIValidationTransientResourceAllocator>( std::move( InAllocator ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDefragmenter>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateDefragmenter () noexcept
{
    return Underlying->CreateDefragmenter().Transform(
        [this] ( TSharedRef<IRHIDefragmenter> &&InDefragmenter ) noexcept -> TSharedRef<IRHIDefragmenter>
    {
        return MakeShared<FRHIValidationDefragmenter>( std::move( InDefragmenter ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBindingLayout>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateBindingLayout ( const FRHIBindingLayoutDesc &InDesc ) noexcept
{
    return Underlying->CreateBindingLayout( InDesc ).Transform(
        [this] ( TSharedRef<IRHIBindingLayout> &&InLayout ) noexcept -> TSharedRef<IRHIBindingLayout>
    {
        return MakeShared<FRHIValidationBindingLayout>( std::move( InLayout ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBindingSet>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateBindingSet ( const TSharedRef<IRHIBindingLayout> &InLayout ) noexcept
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

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDescriptorAllocator>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateDescriptorAllocator ( const FRHIDescriptorAllocatorDesc &InDesc ) noexcept
{
    return Underlying->CreateDescriptorAllocator( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIShader>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateShader ( const FRHIShaderDesc &InDesc ) noexcept
{
    return Underlying->CreateShader( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipelineLayout>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreatePipelineLayout ( const FRHIPipelineLayoutDesc &InDesc ) noexcept
{
    FRHIPipelineLayoutDesc Unwrapped;
    Unwrapped.InlineConstants = InDesc.InlineConstants;
    Unwrapped.BindingLayouts.reserve( InDesc.BindingLayouts.size() );
    for ( const TSharedRef<IRHIBindingLayout> &Layout : InDesc.BindingLayouts )
    {
        const FRHIExpected Owned = State->ValidateObject( *Layout, ERHIValidationObjectKind::BindingLayout );
        if ( not Owned )
        {
            return TUnexpected<FRHIError>( Owned.Error() );
        }
        Unwrapped.BindingLayouts.push_back( UnwrapRHIShared( Layout ) );
    }
    return Underlying->CreatePipelineLayout( Unwrapped );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipeline>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateGraphicsPipeline ( const FRHIGraphicsPipelineDesc &InDesc ) noexcept
{
    return Underlying->CreateGraphicsPipeline( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipeline>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateComputePipeline ( const FRHIComputePipelineDesc &InDesc ) noexcept
{
    return Underlying->CreateComputePipeline( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipelineCache>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreatePipelineCache ( const FRHIPipelineCacheDesc &InDesc ) noexcept
{
    return Underlying->CreatePipelineCache( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHICommandAllocator>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateCommandAllocator ( const ERHIQueueType InQueueType ) noexcept
{
    return Underlying->CreateCommandAllocator( InQueueType )
        .Transform( [this] ( TSharedRef<IRHICommandAllocator> &&InAllocator ) noexcept -> TSharedRef<IRHICommandAllocator>
    {
        return MakeShared<FRHIValidationCommandAllocator>( std::move( InAllocator ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHICommandList>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateCommandList ( const TSharedRef<IRHICommandAllocator> &InAllocator ) noexcept
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

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISemaphore>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateSemaphore ( const FRHISemaphoreDesc &InDesc ) noexcept
{
    return Underlying->CreateSemaphore( InDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISwapchain>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateSwapchain ( const FRHISwapchainDesc &InDesc ) noexcept
{
    return Underlying->CreateSwapchain( InDesc ).Transform(
        [this] ( TSharedRef<IRHISwapchain> &&InSwapchain ) noexcept -> TSharedRef<IRHISwapchain>
    {
        return MakeShared<FRHIValidationSwapchain>( std::move( InSwapchain ), State );
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIQueryPool>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationDevice::CreateQueryPool ( const FRHIQueryPoolDesc &InDesc ) noexcept
{
    return Underlying->CreateQueryPool( InDesc );
}

const Lumen::TSharedPtr<Lumen::RHI::FRHIValidationState> &Lumen::RHI::FRHIValidationDevice::GetState () const noexcept
{
    return State;
}
