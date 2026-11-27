/**
 * @file OpenGLDevice.cpp
 * @brief Implements the negotiated OpenGL device factories and logical queues.
 */

#include "Templates/Optional.hpp"
#include <RHIOpenGL/Device/OpenGLDevice.hpp>

#include <Algo/Find.hpp>
#include <Algo/ForEach.hpp>

#include <RHI/Bindings/RHIBindingLayoutDesc.hpp>
#include <RHI/Bindings/RHIDescriptorAllocatorDesc.hpp>
#include <RHI/Caches/RHIPipelineCacheDesc.hpp>
#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Memory/RHIHeapDesc.hpp>
#include <RHI/Pipelines/RHIComputePipelineDesc.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <RHI/Presentation/RHISwapchainCapabilities.hpp>
#include <RHI/Presentation/RHISwapchainDesc.hpp>
#include <RHI/Queries/RHIQueryPoolDesc.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Resources/RHISamplerDesc.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Resources/RHITextureViewDesc.hpp>
#include <RHI/Shaders/RHIShaderDesc.hpp>
#include <RHI/Synchronization/RHISemaphoreDesc.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Bindings/OpenGLBindingLayout.hpp>
#include <RHIOpenGL/Bindings/OpenGLBindingSet.hpp>
#include <RHIOpenGL/Bindings/OpenGLDescriptorAllocator.hpp>
#include <RHIOpenGL/Caches/OpenGLPipelineCache.hpp>
#include <RHIOpenGL/Commands/OpenGLCommandAllocator.hpp>
#include <RHIOpenGL/Commands/OpenGLCommandList.hpp>
#include <RHIOpenGL/Memory/OpenGLHeap.hpp>
#include <RHIOpenGL/Memory/OpenGLMemoryObjects.hpp>
#include <RHIOpenGL/Memory/OpenGLStagingAllocator.hpp>
#include <RHIOpenGL/Pipelines/OpenGLComputePipeline.hpp>
#include <RHIOpenGL/Pipelines/OpenGLGraphicsPipeline.hpp>
#include <RHIOpenGL/Pipelines/OpenGLPipelineLayout.hpp>
#include <RHIOpenGL/Presentation/OpenGLSwapchain.hpp>
#include <RHIOpenGL/Queue/OpenGLQueue.hpp>
#include <RHIOpenGL/Resources/OpenGLBuffer.hpp>
#include <RHIOpenGL/Resources/OpenGLSampler.hpp>
#include <RHIOpenGL/Resources/OpenGLTexture.hpp>
#include <RHIOpenGL/Resources/OpenGLTextureView.hpp>
#include <RHIOpenGL/Shaders/OpenGLShader.hpp>
#include <RHIOpenGL/Synchronization/OpenGLQueryPool.hpp>
#include <RHIOpenGL/Synchronization/OpenGLSemaphore.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

/**
 * File-local helpers
 */

namespace
{

/** @return The presentation contract actually delivered by an OpenGL 4.6 context. */
[[nodiscard]] Lumen::RHI::FRHISwapchainCapabilities MakeSwapchainCapabilities () noexcept
{
    using Lumen::RHI::ERHIFormat;
    using Lumen::RHI::ERHIPresentMode;
    Lumen::RHI::FRHISwapchainCapabilities Capabilities;
    Capabilities.Formats       = { ERHIFormat::RGBA8Unorm, ERHIFormat::BGRA8Unorm, ERHIFormat::RGBA16Float };
    Capabilities.ColorSpaces   = { Lumen::RHI::ERHIColorSpace::SrgbNonLinear, Lumen::RHI::ERHIColorSpace::ExtendedSrgbLinear };
    Capabilities.PresentModes  = { ERHIPresentMode::Fifo, ERHIPresentMode::Immediate };
    Capabilities.MinExtent     = { 1U, 1U, 1U };
    Capabilities.MaxExtent     = { 16384U, 16384U, 1U };
    Capabilities.MinImageCount = 2U;
    Capabilities.MaxImageCount = 8U;
    return Capabilities;
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDevice>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::Create ( const RHI::FRHIAdapterInfo &InAdapter,
                                          const RHI::FRHIDeviceDesc &InDesc,
                                          const FOpenGLNativeWindowDesc *InNativeWindow ) noexcept
{
    LUMEN_EXPECT( Context, FOpenGLDeviceContext::Create( InAdapter, InDesc.bEnableDebug, InDesc.DebugSink, InNativeWindow ) );

    TSharedRef<FOpenGLDevice> Device = MakeShared<FOpenGLDevice>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( Context ) );
    Algo::ForEach( InDesc.Queues, [&Device] ( const RHI::FRHIQueueRequest &InRequest ) noexcept
    {
        for ( UInt32 Index = 0U; Index < InRequest.Count; ++Index )
        {
            Device->AddQueue( InRequest.Type, Index );
        }
    } );
    return Device;
}

void Lumen::RHIOpenGL::FOpenGLDevice::AddQueue ( const RHI::ERHIQueueType InType, const UInt32 InIndex ) noexcept
{
    TSharedRef<FOpenGLQueue> Queue = FOpenGLQueue::Create( NextObjectId(), Context, InType, InIndex );
    TrackObject( *Queue );

    FQueueEntry QueueEntry = { .Type = InType, .Index = InIndex, .Queue = static_cast<TSharedRef<RHI::IRHIQueue> &&>( Queue ) };
    Queues.push_back( QueueEntry );
}

Lumen::RHIOpenGL::FOpenGLDevice::FOpenGLDevice ( TSharedRef<FOpenGLDeviceContext> InContext ) noexcept
    : RHI::FRHIDeviceBase( InContext->GetCapabilities(), InContext->GetLimits(), GetOpenGLBackendTypeTag<FOpenGLDevice>() ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLDevice::~FOpenGLDevice () noexcept
{
    Context->DetachDebugSink();
    if ( GetStatus() != RHI::ERHIDeviceStatus::Lost )
    {
        Context->MarkClosed();
    }
    /* Stop and join while this device still owns the context: otherwise the final
     * reference can be released on the executor itself and destroy it mid-replay. */
    Context->GetExecutor()->Shutdown();
    SetStatus( RHI::ERHIDeviceStatus::Closed );
}

void Lumen::RHIOpenGL::FOpenGLDevice::InjectDeviceLost () noexcept
{
    SetStatus( RHI::ERHIDeviceStatus::Lost );
    Context->MarkLost();
    Context->GetExecutor()->EmitDebugMessage( RHI::FRHIDebugMessage(
        RHI::ERHIDebugSeverity::Error, RHI::ERHIDebugCategory::General, 0U, "OpenGL device loss was injected." ) );
}

Lumen::RHI::ERHIDeviceStatus Lumen::RHIOpenGL::FOpenGLDevice::GetStatus () const noexcept
{
    return Context->IsDeviceLost() ? RHI::ERHIDeviceStatus::Lost : RHI::FRHIDeviceBase::GetStatus();
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLDevice::WaitIdle () noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "WaitIdle" ) );
    return {};
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIQueue>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::GetQueue ( const RHI::ERHIQueueType InType, const UInt32 InIndex ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "GetQueue" ) );

    if ( static_cast<USize>( InType ) >= static_cast<USize>( RHI::ERHIQueueType::Count ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidQueue( "Invalid ERHIQueueType value." ) );
    }
    const FQueueEntry *Found = Algo::FindIf( Queues, [InType, InIndex] ( const FQueueEntry &InEntry ) noexcept -> Bool
    {
        return InEntry.Type == InType and InEntry.Index == InIndex;
    } );
    if ( Found != nullptr )
    {
        return Found->Queue;
    }
    return TUnexpected(
        RHI::MakeRHINotFound( RHI::ERHIErrorCondition::Queue, "Requested OpenGL queue was not created on this device." ) );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateBuffer ( const RHI::FRHIBufferDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateBuffer" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Buffer, FOpenGLBuffer::Create( Context, InDesc, NextObjectId() ) );

    TrackObject( *Buffer );
    return Buffer;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreatePlacedBuffer ( const RHI::FRHIBufferDesc &InDesc,
                                                      TSharedRef<RHI::IRHIHeap> InHeap,
                                                      const RHI::FRHIAllocation &InAllocation ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreatePlacedBuffer" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Buffer, FOpenGLBuffer::CreatePlaced( Context, InDesc, static_cast<TSharedRef<RHI::IRHIHeap> &&>( InHeap ),
                                                       InAllocation, NextObjectId() ) );

    TrackObject( *Buffer );
    return Buffer;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateTexture ( const RHI::FRHITextureDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateTexture" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Texture, FOpenGLTexture::Create( Context, InDesc, NextObjectId() ) );

    TrackObject( *Texture );
    return Texture;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreatePlacedTexture ( const RHI::FRHITextureDesc &InDesc,
                                                       TSharedRef<RHI::IRHIHeap> InHeap,
                                                       const RHI::FRHIAllocation &InAllocation ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreatePlacedTexture" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Texture, FOpenGLTexture::CreatePlaced( Context, InDesc, static_cast<TSharedRef<RHI::IRHIHeap> &&>( InHeap ),
                                                         InAllocation, NextObjectId() ) );

    TrackObject( *Texture );
    return Texture;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITextureView>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateTextureView ( const RHI::FRHITextureViewDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateTextureView" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( View, FOpenGLTextureView::Create( Context, InDesc, NextObjectId() ) );

    TrackObject( *View );
    return View;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISampler>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateSampler ( const RHI::FRHISamplerDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateSampler" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Sampler, FOpenGLSampler::Create( Context, InDesc, NextObjectId() ) );

    TrackObject( *Sampler );
    return Sampler;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIHeap>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateHeap ( const RHI::FRHIHeapDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateHeap" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Heap, FOpenGLHeap::Create( Context, InDesc, NextObjectId() ) );

    TrackObject( *Heap );
    return Heap;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBufferPool>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateBufferPool () noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateBufferPool" ) );
    TSharedRef<TAtomic<UInt64>> Counter = MakeShared<TAtomic<UInt64>>( 1ULL << 40U );

    TFunction<TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>( const RHI::FRHIBufferDesc & )> Creator =
        [LocalContext = Context,
         Counter] ( const RHI::FRHIBufferDesc &InDesc ) noexcept -> TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>
    {
        LUMEN_TRY_OPTIONAL_ERROR( LocalContext->GetUnavailableError( "CreateBufferPool" ) );
        LUMEN_EXPECT( Created, FOpenGLBuffer::Create( LocalContext, InDesc, RHI::FRHIObjectId{ Counter->fetch_add( 1ULL ) } ) );
        return StaticCastSharedRef<RHI::IRHIBuffer>( Created );
    };

    TSharedRef<FOpenGLBufferPool> Pool =
        MakeShared<FOpenGLBufferPool>( static_cast<decltype( Creator ) &&>( Creator ), NextObjectId() );
    TrackObject( *Pool );
    return Pool;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIUploadAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateUploadAllocator ( const UInt64 InCapacity ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateUploadAllocator" ) );
    if ( InCapacity == 0ULL )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Upload allocator capacity must be nonzero." ) );
    }

    LUMEN_EXPECT( Allocator, FOpenGLUploadAllocator::Create( Context, InCapacity, NextObjectId(), NextObjectId() ) );
    TrackObject( *Allocator );
    return Allocator;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIReadbackAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateReadbackAllocator ( const UInt64 InCapacity ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateReadbackAllocator" ) );
    if ( InCapacity == 0ULL )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Readback allocator capacity must be nonzero." ) );
    }

    LUMEN_EXPECT( Allocator, FOpenGLReadbackAllocator::Create( Context, InCapacity, NextObjectId(), NextObjectId() ) );
    TrackObject( *Allocator );
    return Allocator;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITransientResourceAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateTransientResourceAllocator () noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateTransientResourceAllocator" ) );
    TSharedRef<FOpenGLTransientResourceAllocator> Allocator =
        MakeShared<FOpenGLTransientResourceAllocator>( NextObjectId(), Context->GetLimits().MinBufferAlignment );
    TrackObject( *Allocator );
    return Allocator;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDefragmenter>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateDefragmenter () noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateDefragmenter" ) );
    TSharedRef<FOpenGLDefragmenter> Defragmenter = MakeShared<FOpenGLDefragmenter>( Context, NextObjectId() );
    TrackObject( *Defragmenter );
    return Defragmenter;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBindingLayout>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateBindingLayout ( const RHI::FRHIBindingLayoutDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateBindingLayout" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Layout, FOpenGLBindingLayout::Create( Context, InDesc, NextObjectId() ) );
    TrackObject( *Layout );
    return Layout;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBindingSet>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateBindingSet ( const TSharedRef<RHI::IRHIBindingLayout> &InLayout ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateBindingSet" ) );
    LUMEN_EXPECT( Set, FOpenGLBindingSet::Create( Context, InLayout, NextObjectId() ) );
    TrackObject( *Set );
    return Set;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDescriptorAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateDescriptorAllocator ( const RHI::FRHIDescriptorAllocatorDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateDescriptorAllocator" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Allocator, FOpenGLDescriptorAllocator::Create( InDesc, NextObjectId() ) );
    TrackObject( *Allocator );
    return Allocator;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIShader>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateShader ( const RHI::FRHIShaderDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateShader" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Shader, FOpenGLShader::Create( Context, InDesc, NextObjectId() ) );
    TrackObject( *Shader );
    return Shader;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipelineLayout>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreatePipelineLayout ( const RHI::FRHIPipelineLayoutDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreatePipelineLayout" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Layout, FOpenGLPipelineLayout::Create( Context, InDesc, NextObjectId() ) );
    TrackObject( *Layout );
    return Layout;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipeline>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateGraphicsPipeline ( const RHI::FRHIGraphicsPipelineDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateGraphicsPipeline" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Pipeline, FOpenGLGraphicsPipeline::Create( Context, InDesc, NextObjectId() ) );
    TrackObject( *Pipeline );
    return Pipeline;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipeline>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateComputePipeline ( const RHI::FRHIComputePipelineDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateComputePipeline" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Pipeline, FOpenGLComputePipeline::Create( Context, InDesc, NextObjectId() ) );
    TrackObject( *Pipeline );
    return Pipeline;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipelineCache>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreatePipelineCache ( const RHI::FRHIPipelineCacheDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreatePipelineCache" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Cache, FOpenGLPipelineCache::Create( InDesc, NextObjectId() ) );

    TrackObject( *Cache );
    return Cache;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHICommandAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateCommandAllocator ( const RHI::ERHIQueueType InQueueType ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateCommandAllocator" ) );
    if ( static_cast<USize>( InQueueType ) >= static_cast<USize>( RHI::ERHIQueueType::Count ) )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Command, "Invalid queue type for a command allocator." ) );
    }
    TSharedRef<FOpenGLCommandAllocator> Allocator = MakeShared<FOpenGLCommandAllocator>( InQueueType, NextObjectId() );
    TrackObject( *Allocator );
    return Allocator;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHICommandList>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateCommandList ( const TSharedRef<RHI::IRHICommandAllocator> &InAllocator ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateCommandList" ) );
    if ( not RHI::TryRHIBackendCast<FOpenGLCommandAllocator>( *InAllocator ) )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Command, "Command allocator belongs to another backend." ) );
    }
    TSharedRef<FOpenGLCommandAllocator> NativeAllocator = StaticCastSharedRef<FOpenGLCommandAllocator>( InAllocator );

    LUMEN_EXPECT( List, FOpenGLCommandList::Create( static_cast<TSharedRef<FOpenGLCommandAllocator> &&>( NativeAllocator ),
                                                    Context, NextObjectId() ) );
    TrackObject( *List );
    return List;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISemaphore>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateSemaphore ( const RHI::FRHISemaphoreDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateSemaphore" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Semaphore, FOpenGLSemaphore::Create( Context, InDesc, NextObjectId() ) );
    TrackObject( *Semaphore );
    return Semaphore;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISwapchain>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateSwapchain ( const RHI::FRHISwapchainDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateSwapchain" ) );
    if ( not Context->GetCapabilities().Supports( RHI::ERHICapabilityFlags::Presentation ) )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Presentation,
                                                     "Presentation was not negotiated by this adapter." ) );
    }
    const RHI::FRHISwapchainCapabilities Negotiated = MakeSwapchainCapabilities();
    LUMEN_EXPECT( Validated, InDesc.Validate( Negotiated ) );
    LUMEN_EXPECT( Swapchain, FOpenGLSwapchain::Create( Context, InDesc, Negotiated, NextObjectId() ) );
    TrackObject( *Swapchain );
    return Swapchain;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIQueryPool>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateQueryPool ( const RHI::FRHIQueryPoolDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "CreateQueryPool" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    RHI::ERHICapabilityFlags Required = RHI::ERHICapabilityFlags::OcclusionQueries;
    if ( InDesc.Type == RHI::ERHIQueryType::Timestamp )
    {
        Required = RHI::ERHICapabilityFlags::TimestampQueries;
    }
    else if ( InDesc.Type == RHI::ERHIQueryType::PipelineStatistics )
    {
        Required = RHI::ERHICapabilityFlags::PipelineStatistics;
    }
    if ( not Context->GetCapabilities().Supports( Required ) )
    {
        return TUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Query, "The requested query type was not negotiated." ) );
    }
    LUMEN_EXPECT( Pool, FOpenGLQueryPool::Create( Context, InDesc, NextObjectId() ) );
    TrackObject( *Pool );
    return Pool;
}
