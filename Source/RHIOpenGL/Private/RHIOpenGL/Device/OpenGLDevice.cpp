/**
 * @file OpenGLDevice.cpp
 * @brief Implements the M0 OpenGL device surface and structured stubs.
 */

#include <RHIOpenGL/Device/OpenGLDevice.hpp>

#include <RHI/Bindings/RHIBindingLayoutDesc.hpp>
#include <RHI/Bindings/RHIDescriptorAllocatorDesc.hpp>
#include <RHI/Caches/RHIPipelineCacheDesc.hpp>
#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Memory/RHIHeapDesc.hpp>
#include <RHI/Pipelines/RHIComputePipelineDesc.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <RHI/Presentation/RHISwapchainDesc.hpp>
#include <RHI/Queries/RHIQueryPoolDesc.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Resources/RHISamplerDesc.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Resources/RHITextureViewDesc.hpp>
#include <RHI/Shaders/RHIShaderDesc.hpp>
#include <RHI/Synchronization/RHISemaphoreDesc.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Commands/OpenGLCommandAllocator.hpp>
#include <RHIOpenGL/Commands/OpenGLCommandList.hpp>
#include <RHIOpenGL/Memory/OpenGLHeap.hpp>
#include <RHIOpenGL/Memory/OpenGLMemoryObjects.hpp>
#include <RHIOpenGL/Memory/OpenGLStagingAllocator.hpp>
#include <RHIOpenGL/Queue/OpenGLQueue.hpp>
#include <RHIOpenGL/Resources/OpenGLBuffer.hpp>
#include <RHIOpenGL/Resources/OpenGLSampler.hpp>
#include <RHIOpenGL/Resources/OpenGLTexture.hpp>
#include <RHIOpenGL/Resources/OpenGLTextureView.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDevice>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::Create ( const RHI::FRHIAdapterInfo &InAdapter, const RHI::FRHIDeviceDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( Context, FOpenGLDeviceContext::Create( InAdapter, InDesc.bEnableDebug ) );

    TSharedRef<FOpenGLDevice> Device = MakeShared<FOpenGLDevice>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( Context ) );
    for ( const RHI::FRHIQueueRequest &Request : InDesc.Queues )
    {
        for ( UInt32 Index = 0U; Index < Request.Count; ++Index )
        {
            Device->AddQueue( Request.Type, Index );
        }
    }
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
    if ( GetStatus() != RHI::ERHIDeviceStatus::Lost )
    {
        Context->MarkClosed();
    }
    SetStatus( RHI::ERHIDeviceStatus::Closed );
}

void Lumen::RHIOpenGL::FOpenGLDevice::InjectDeviceLost () noexcept
{
    SetStatus( RHI::ERHIDeviceStatus::Lost );
    Context->MarkLost();
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLDevice::WaitIdle () noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "WaitIdle" ) );
    return {};
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIQueue>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::GetQueue ( const RHI::ERHIQueueType InType, const UInt32 InIndex ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "GetQueue" ) );

    if ( static_cast<USize>( InType ) >= static_cast<USize>( RHI::ERHIQueueType::Count ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidQueue( "Invalid ERHIQueueType value." ) );
    }
    for ( const FQueueEntry &Entry : Queues )
    {
        if ( Entry.Type == InType and Entry.Index == InIndex )
        {
            return Entry.Queue;
        }
    }
    return TUnexpected(
        RHI::MakeRHINotFound( RHI::ERHIErrorCondition::Queue, "Requested OpenGL queue was not created on this device." ) );
}

#define LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( OperationName, Condition )                                                           \
    do                                                                                                                           \
    {                                                                                                                            \
        LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( OperationName ) );                                                        \
        return TUnexpected( RHI::MakeRHIUnsupported( Condition, OperationName " is not implemented in RHIOpenGL M0 yet." ) );    \
    } while ( false )

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateBuffer ( const RHI::FRHIBufferDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateBuffer" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    TExpected<TSharedRef<FOpenGLBuffer>, RHI::FRHIError> Buffer = FOpenGLBuffer::Create( Context, InDesc, NextObjectId() );
    if ( not Buffer )
    {
        return TUnexpected( Buffer.Error() );
    }
    TrackObject( **Buffer );
    return *Buffer;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreatePlacedBuffer ( const RHI::FRHIBufferDesc &InDesc,
                                                      TSharedRef<RHI::IRHIHeap> InHeap,
                                                      const RHI::FRHIAllocation &InAllocation ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreatePlacedBuffer" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    TExpected<TSharedRef<FOpenGLBuffer>, RHI::FRHIError> Buffer = FOpenGLBuffer::CreatePlaced(
        Context, InDesc, static_cast<TSharedRef<RHI::IRHIHeap> &&>( InHeap ), InAllocation, NextObjectId() );
    if ( not Buffer )
    {
        return TUnexpected( Buffer.Error() );
    }
    TrackObject( **Buffer );
    return *Buffer;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateTexture ( const RHI::FRHITextureDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateTexture" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    TExpected<TSharedRef<FOpenGLTexture>, RHI::FRHIError> Texture = FOpenGLTexture::Create( Context, InDesc, NextObjectId() );
    if ( not Texture )
    {
        return TUnexpected( Texture.Error() );
    }
    TrackObject( **Texture );
    return *Texture;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreatePlacedTexture ( const RHI::FRHITextureDesc &InDesc,
                                                       TSharedRef<RHI::IRHIHeap> InHeap,
                                                       const RHI::FRHIAllocation &InAllocation ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreatePlacedTexture" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    TExpected<TSharedRef<FOpenGLTexture>, RHI::FRHIError> Texture = FOpenGLTexture::CreatePlaced(
        Context, InDesc, static_cast<TSharedRef<RHI::IRHIHeap> &&>( InHeap ), InAllocation, NextObjectId() );
    if ( not Texture )
    {
        return TUnexpected( Texture.Error() );
    }
    TrackObject( **Texture );
    return *Texture;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITextureView>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateTextureView ( const RHI::FRHITextureViewDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateTextureView" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    TExpected<TSharedRef<FOpenGLTextureView>, RHI::FRHIError> View =
        FOpenGLTextureView::Create( Context, InDesc, NextObjectId() );
    if ( not View )
    {
        return TUnexpected( View.Error() );
    }
    TrackObject( **View );
    return *View;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISampler>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateSampler ( const RHI::FRHISamplerDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateSampler" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    TExpected<TSharedRef<FOpenGLSampler>, RHI::FRHIError> Sampler = FOpenGLSampler::Create( Context, InDesc, NextObjectId() );
    if ( not Sampler )
    {
        return TUnexpected( Sampler.Error() );
    }
    TrackObject( **Sampler );
    return *Sampler;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIHeap>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateHeap ( const RHI::FRHIHeapDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateHeap" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    TExpected<TSharedRef<FOpenGLHeap>, RHI::FRHIError> Heap = FOpenGLHeap::Create( Context, InDesc, NextObjectId() );
    if ( not Heap )
    {
        return TUnexpected( Heap.Error() );
    }
    TrackObject( **Heap );
    return *Heap;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBufferPool>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateBufferPool () noexcept
{
    TSharedRef<TAtomic<UInt64>> Counter = MakeShared<TAtomic<UInt64>>( 1ULL << 40U );
    TFunction<TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>( const RHI::FRHIBufferDesc & )> Creator =
        [LocalContext = Context,
         Counter] ( const RHI::FRHIBufferDesc &InDesc ) noexcept -> TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>
    {
        TExpected<TSharedRef<FOpenGLBuffer>, RHI::FRHIError> Created =
            FOpenGLBuffer::Create( LocalContext, InDesc, RHI::FRHIObjectId{ Counter->fetch_add( 1ULL ) } );
        if ( not Created )
        {
            return TUnexpected( Created.Error() );
        }
        return StaticCastSharedRef<RHI::IRHIBuffer>( *Created );
    };
    TSharedRef<FOpenGLBufferPool> Pool =
        MakeShared<FOpenGLBufferPool>( static_cast<decltype( Creator ) &&>( Creator ), NextObjectId() );
    TrackObject( *Pool );
    return Pool;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIUploadAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateUploadAllocator ( const UInt64 InCapacity ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateUploadAllocator" ) );
    if ( InCapacity == 0ULL )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Upload allocator capacity must be nonzero." ) );
    }
    TExpected<TSharedRef<FOpenGLUploadAllocator>, RHI::FRHIError> Allocator =
        FOpenGLUploadAllocator::Create( Context, InCapacity, NextObjectId(), NextObjectId() );
    if ( not Allocator )
    {
        return TUnexpected( Allocator.Error() );
    }
    TrackObject( **Allocator );
    return *Allocator;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIReadbackAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateReadbackAllocator ( const UInt64 InCapacity ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateReadbackAllocator" ) );
    if ( InCapacity == 0ULL )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Readback allocator capacity must be nonzero." ) );
    }
    TExpected<TSharedRef<FOpenGLReadbackAllocator>, RHI::FRHIError> Allocator =
        FOpenGLReadbackAllocator::Create( Context, InCapacity, NextObjectId(), NextObjectId() );
    if ( not Allocator )
    {
        return TUnexpected( Allocator.Error() );
    }
    TrackObject( **Allocator );
    return *Allocator;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITransientResourceAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateTransientResourceAllocator () noexcept
{
    TSharedRef<FOpenGLTransientResourceAllocator> Allocator =
        MakeShared<FOpenGLTransientResourceAllocator>( NextObjectId(), Context->GetLimits().MinBufferAlignment );
    TrackObject( *Allocator );
    return Allocator;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDefragmenter>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateDefragmenter () noexcept
{
    TSharedRef<FOpenGLDefragmenter> Defragmenter = MakeShared<FOpenGLDefragmenter>( NextObjectId() );
    TrackObject( *Defragmenter );
    return Defragmenter;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBindingLayout>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateBindingLayout ( const RHI::FRHIBindingLayoutDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateBindingLayout" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( "CreateBindingLayout", RHI::ERHIErrorCondition::Binding );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBindingSet>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateBindingSet ( const TSharedRef<RHI::IRHIBindingLayout> & ) noexcept
{
    LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( "CreateBindingSet", RHI::ERHIErrorCondition::Binding );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDescriptorAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateDescriptorAllocator ( const RHI::FRHIDescriptorAllocatorDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateDescriptorAllocator" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( "CreateDescriptorAllocator", RHI::ERHIErrorCondition::Binding );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIShader>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateShader ( const RHI::FRHIShaderDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateShader" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( "CreateShader", RHI::ERHIErrorCondition::Shader );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipelineLayout>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreatePipelineLayout ( const RHI::FRHIPipelineLayoutDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreatePipelineLayout" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( "CreatePipelineLayout", RHI::ERHIErrorCondition::Pipeline );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipeline>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateGraphicsPipeline ( const RHI::FRHIGraphicsPipelineDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateGraphicsPipeline" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( "CreateGraphicsPipeline", RHI::ERHIErrorCondition::Pipeline );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipeline>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateComputePipeline ( const RHI::FRHIComputePipelineDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateComputePipeline" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( "CreateComputePipeline", RHI::ERHIErrorCondition::Pipeline );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipelineCache>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreatePipelineCache ( const RHI::FRHIPipelineCacheDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreatePipelineCache" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( "CreatePipelineCache", RHI::ERHIErrorCondition::Cache );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHICommandAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateCommandAllocator ( const RHI::ERHIQueueType InQueueType ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateCommandAllocator" ) );
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
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateCommandList" ) );
    if ( not RHI::TryRHIBackendCast<FOpenGLCommandAllocator>( *InAllocator ) )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Command, "Command allocator belongs to another backend." ) );
    }
    TSharedRef<FOpenGLCommandAllocator> NativeAllocator            = StaticCastSharedRef<FOpenGLCommandAllocator>( InAllocator );
    TExpected<TSharedRef<FOpenGLCommandList>, RHI::FRHIError> List = FOpenGLCommandList::Create(
        static_cast<TSharedRef<FOpenGLCommandAllocator> &&>( NativeAllocator ), Context, NextObjectId() );
    if ( not List )
    {
        return TUnexpected( List.Error() );
    }
    TrackObject( **List );
    return *List;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISemaphore>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateSemaphore ( const RHI::FRHISemaphoreDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateSemaphore" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( "CreateSemaphore", RHI::ERHIErrorCondition::Synchronization );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISwapchain>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateSwapchain ( const RHI::FRHISwapchainDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateSwapchain" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( "CreateSwapchain", RHI::ERHIErrorCondition::Presentation );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIQueryPool>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDevice::CreateQueryPool ( const RHI::FRHIQueryPoolDesc &InDesc ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "CreateQueryPool" ) );
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_OPENGL_UNIMPLEMENTED_FACTORY( "CreateQueryPool", RHI::ERHIErrorCondition::Query );
}

#undef LUMEN_OPENGL_UNIMPLEMENTED_FACTORY
