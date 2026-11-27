/**
 * @file OpenGLCommandList.cpp
 * @brief Implements validated command recording for replay on the GL Executor.
 */

#include <RHIOpenGL/Commands/OpenGLCommandList.hpp>

#include <Algo/Find.hpp>
#include <Algo/Sort.hpp>

#include <HAL/LockGuard.hpp>

#include <RHI/Commands/RHICommandListState.hpp>
#include <RHI/Commands/RHICopyDescs.hpp>
#include <RHI/Commands/RHIDrawArguments.hpp>
#include <RHI/Commands/RHIRenderingDesc.hpp>
#include <RHI/Commands/RHITransition.hpp>
#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Pipelines/IRHIPipeline.hpp>
#include <RHI/Pipelines/RHIInlineConstantRange.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIFormatInfo.hpp>
#include <RHI/Types/RHIHash.hpp>

#include <RHI/Validation/RHIValidation.hpp>

#include <RHIOpenGL/Bindings/OpenGLBindingSet.hpp>
#include <RHIOpenGL/Commands/OpenGLCommandStreamOpcodes.hpp>
#include <RHIOpenGL/Pipelines/OpenGLComputePipeline.hpp>
#include <RHIOpenGL/Pipelines/OpenGLGraphicsPipeline.hpp>
#include <RHIOpenGL/Pipelines/OpenGLPipelineLayout.hpp>
#include <RHIOpenGL/Resources/OpenGLBuffer.hpp>
#include <RHIOpenGL/Resources/OpenGLSampler.hpp>
#include <RHIOpenGL/Resources/OpenGLTexture.hpp>
#include <RHIOpenGL/Resources/OpenGLTextureView.hpp>
#include <RHIOpenGL/Synchronization/OpenGLQueryPool.hpp>
#include <RHIOpenGL/Types/OpenGLFormatMapping.hpp>

/**
 * File-local helpers
 */

namespace
{

[[nodiscard]] Lumen::RHI::FRHIExpected InvalidCommand ( const Lumen::FStringView InMessage ) noexcept
{
    return Lumen::TUnexpected<Lumen::RHI::FRHIError>(
        Lumen::RHI::MakeRHIInvalidArgument( Lumen::RHI::ERHIErrorCondition::Command, InMessage ) );
}

[[nodiscard]] Lumen::Bool SupportsRendering ( const Lumen::RHI::ERHIQueueType InQueueType ) noexcept
{
    return InQueueType == Lumen::RHI::ERHIQueueType::Graphics;
}

Lumen::Bool SupportsCompute ( const Lumen::RHI::ERHIQueueType InQueueType ) noexcept
{
    return InQueueType == Lumen::RHI::ERHIQueueType::Graphics or InQueueType == Lumen::RHI::ERHIQueueType::Compute;
}

Lumen::UInt32 RegisterQueryPool ( Lumen::TVector<Lumen::TSharedPtr<Lumen::RHI::IRHIQueryPool>> &InPools,
                                  const Lumen::TSharedRef<Lumen::RHI::IRHIQueryPool> &InPool ) noexcept
{
    const Lumen::TSharedPtr<Lumen::RHI::IRHIQueryPool> *Found = Lumen::Algo::FindIf(
        InPools, [&InPool] ( const Lumen::TSharedPtr<Lumen::RHI::IRHIQueryPool> &InCandidate ) noexcept -> Lumen::Bool
    {
        return InCandidate.Get() == InPool.Get();
    } );
    if ( Found != nullptr )
    {
        return static_cast<Lumen::UInt32>( Found - InPools.data() );
    }
    InPools.emplace_back( InPool );
    return static_cast<Lumen::UInt32>( InPools.size() - 1U );
}

/** @brief Resolves a descriptor slot into a deterministic per-class array index. */
Lumen::Bool ResolveBindingClass ( const Lumen::RHIOpenGL::FOpenGLBindingLayout &InLayout,
                                  const Lumen::UInt32 InSlot,
                                  const Lumen::UInt32 InArrayElement,
                                  Lumen::RHI::ERHIBindingType &OutType,
                                  Lumen::UInt32 &OutClassIndex ) noexcept
{
    Lumen::TVector<Lumen::RHI::FRHIBindingLayoutEntry> Entries = InLayout.GetDesc().Entries;
    Lumen::Algo::Sort( Entries,
                       [] ( const Lumen::RHI::FRHIBindingLayoutEntry &InLeft,
                            const Lumen::RHI::FRHIBindingLayoutEntry &InRight ) noexcept -> Lumen::Bool
    {
        return InLeft.Slot < InRight.Slot;
    } );
    Lumen::UInt32 UniformCount = 0U;
    Lumen::UInt32 StorageCount = 0U;
    Lumen::UInt32 TextureCount = 0U;
    Lumen::UInt32 ImageCount   = 0U;
    for ( const Lumen::RHI::FRHIBindingLayoutEntry &Entry : Entries )
    {
        Lumen::UInt32 *ClassCount = nullptr;
        switch ( Entry.Type )
        {
        case Lumen::RHI::ERHIBindingType::ConstantBuffer:
            ClassCount = &UniformCount;
            break;
        case Lumen::RHI::ERHIBindingType::ReadOnlyBuffer:
        case Lumen::RHI::ERHIBindingType::ReadWriteBuffer:
            ClassCount = &StorageCount;
            break;
        case Lumen::RHI::ERHIBindingType::Texture:
        case Lumen::RHI::ERHIBindingType::Sampler:
        case Lumen::RHI::ERHIBindingType::CombinedTextureSampler:
            ClassCount = &TextureCount;
            break;
        case Lumen::RHI::ERHIBindingType::StorageTexture:
            ClassCount = &ImageCount;
            break;
        default:
            return false;
        }
        if ( Entry.Slot == InSlot )
        {
            OutType       = Entry.Type;
            OutClassIndex = *ClassCount + InArrayElement;
            return true;
        }
        *ClassCount += Entry.Count;
    }
    return false;
}

Lumen::RHIOpenGL::GLbitfield BarrierBitsForState ( const Lumen::RHI::ERHIResourceState InState ) noexcept
{
    switch ( InState )
    {
    case Lumen::RHI::ERHIResourceState::CopySource:
        return Lumen::RHIOpenGL::GlShaderStorageBarrierBit | Lumen::RHIOpenGL::GlBufferUpdateBarrierBit;
    case Lumen::RHI::ERHIResourceState::CopyDestination:
        return Lumen::RHIOpenGL::GlBufferUpdateBarrierBit | Lumen::RHIOpenGL::GlFramebufferBarrierBit;
    case Lumen::RHI::ERHIResourceState::ShaderResource:
        return Lumen::RHIOpenGL::GlTextureFetchBarrierBit | Lumen::RHIOpenGL::GlShaderStorageBarrierBit |
               Lumen::RHIOpenGL::GlUniformBarrierBit;
    case Lumen::RHI::ERHIResourceState::UnorderedAccess:
        return Lumen::RHIOpenGL::GlShaderStorageBarrierBit | Lumen::RHIOpenGL::GlShaderImageAccessBarrierBit;
    case Lumen::RHI::ERHIResourceState::RenderTarget:
        return Lumen::RHIOpenGL::GlFramebufferBarrierBit | Lumen::RHIOpenGL::GlColorBufferBit;
    case Lumen::RHI::ERHIResourceState::VertexBuffer:
        return Lumen::RHIOpenGL::GlVertexAttribArrayBarrierBit;
    case Lumen::RHI::ERHIResourceState::IndexBuffer:
        return Lumen::RHIOpenGL::GlElementArrayBarrierBit;
    case Lumen::RHI::ERHIResourceState::IndirectArgument:
        return Lumen::RHIOpenGL::GlCommandBarrierBit;
    default:
        return Lumen::RHIOpenGL::GlClientMappedBufferBarrierBit;
    }
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLCommandList>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLCommandList::Create ( TSharedRef<FOpenGLCommandAllocator> InAllocator,
                                               TSharedRef<FOpenGLDeviceContext> InContext,
                                               const RHI::FRHIObjectId InId ) noexcept
{
    TSharedRef<FOpenGLCommandList> List =
        MakeShared<FOpenGLCommandList>( static_cast<TSharedRef<FOpenGLCommandAllocator> &&>( InAllocator ),
                                        static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InId );
    InAllocator->TrackList( *List );
    return List;
}

Lumen::RHIOpenGL::FOpenGLCommandList::FOpenGLCommandList ( TSharedRef<FOpenGLCommandAllocator> InAllocator,
                                                           TSharedRef<FOpenGLDeviceContext> InContext,
                                                           const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLCommandList>( InId ) ),
      Allocator( static_cast<TSharedRef<FOpenGLCommandAllocator> &&>( InAllocator ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLCommandList::~FOpenGLCommandList () noexcept
{
    Allocator->UntrackList( *this );
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLCommandList );

Lumen::RHI::ERHIQueueType Lumen::RHIOpenGL::FOpenGLCommandList::GetQueueType () const noexcept
{
    return Allocator->GetQueueType();
}

Lumen::RHI::ERHICommandListState Lumen::RHIOpenGL::FOpenGLCommandList::GetState () const noexcept
{
    HAL::TLockGuard Lock( Mutex );
    return State;
}

Lumen::RHI::FRHITimelineValue Lumen::RHIOpenGL::FOpenGLCommandList::GetCompletion () const noexcept
{
    HAL::TLockGuard Lock( Mutex );
    return Completion;
}

const Lumen::RHI::FRHICommandStream &Lumen::RHIOpenGL::FOpenGLCommandList::GetStream () const noexcept
{
    return Stream;
}

const Lumen::TVector<Lumen::TSharedPtr<Lumen::RHI::IRHIQueryPool>> &
Lumen::RHIOpenGL::FOpenGLCommandList::GetQueryPools () const noexcept
{
    return QueryPools;
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::Begin () noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RHI::RHITransitionCommandList( State, RHI::ERHICommandListState::Recording ) );
    Stream.Clear();
    QueryPools.clear();
    bHasPipeline       = false;
    BoundGraphics      = nullptr;
    BoundLayout        = nullptr;
    BoundDynamicStates = RHI::ERHIDynamicState::None;
    bHasIndexBuffer    = false;
    BoundIndexType     = GlUnsignedInt;
    ActiveQueryPool    = nullptr;
    ActiveQueryIndex   = 0U;
    DebugDepth         = 0U;
    Completion         = {};
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::End () noexcept
{
    if ( ActiveQueryPool != nullptr or DebugDepth != 0U )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "A query or debug region is still open." ) );
    }
    LUMEN_TRY_EXPECT_ERROR( RHI::RHITransitionCommandList( State, RHI::ERHICommandListState::Executable ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::Reset () noexcept
{
    HAL::TLockGuard Lock( Mutex );
    if ( State == RHI::ERHICommandListState::Pending or ActiveQueryPool != nullptr or DebugDepth != 0U )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "Command list cannot be reset." ) );
    }
    Stream.Clear();
    QueryPools.clear();
    State              = RHI::ERHICommandListState::Initial;
    Completion         = {};
    bHasPipeline       = false;
    BoundGraphics      = nullptr;
    BoundLayout        = nullptr;
    BoundDynamicStates = RHI::ERHIDynamicState::None;
    bHasIndexBuffer    = false;
    BoundIndexType     = GlUnsignedInt;
    ActiveQueryPool    = nullptr;
    ActiveQueryIndex   = 0U;
    DebugDepth         = 0U;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::MarkPending ( const RHI::FRHITimelineValue InCompletion ) noexcept
{
    HAL::TLockGuard Lock( Mutex );
    if ( not InCompletion.IsValid() or State != RHI::ERHICommandListState::Executable )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Command, "Invalid pending completion." ) );
    }
    Completion = InCompletion;
    State      = RHI::ERHICommandListState::Pending;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::MarkCompleted ( const RHI::FRHITimelineValue InCompleted ) noexcept
{
    HAL::TLockGuard Lock( Mutex );
    if ( State != RHI::ERHICommandListState::Pending or not RHI::IsRHICompletionReached( InCompleted, Completion ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "Completion was not reached." ) );
    }
    State      = RHI::ERHICommandListState::Executable;
    Completion = {};
    return {};
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::Transition ( const TSpan<const RHI::FRHITransition> InTransitions ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( false ) );
    GLbitfield BarrierBits = 0U;
    for ( const RHI::FRHITransition &Transition : InTransitions )
    {
        LUMEN_TRY_EXPECT_ERROR( RHI::ValidateRHITransition( Transition ) );
        const auto *BufferTransition = std::get_if<RHI::FRHIBufferTransition>( &Transition );
        BarrierBits |= ( BufferTransition != nullptr ) ? BarrierBitsForState( BufferTransition->After ) : GlFramebufferBarrierBit;
        if ( BufferTransition != nullptr )
        {
            Stream.Retain( StaticCastSharedPtr<RHI::IRHIObject>( BufferTransition->Buffer ) );
        }
        else
        {
            Stream.Retain( StaticCastSharedPtr<RHI::IRHIObject>( std::get<RHI::FRHITextureTransition>( Transition ).Texture ) );
        }
    }
    const FOpenGLTransitionPayload Payload{ .BarrierBits = BarrierBits };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::Transition ), Payload ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::CopyBuffer ( TSharedRef<RHI::IRHIBuffer> InSource,
                                                                            TSharedRef<RHI::IRHIBuffer> InDestination,
                                                                            const RHI::FRHIBufferCopy &InCopy ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    const Bool bSameBuffer = InSource.Get() == InDestination.Get();
    LUMEN_TRY_EXPECT_ERROR( InCopy.Validate( InSource->GetDesc().Size, InDestination->GetDesc().Size, bSameBuffer ) );
    if ( not HasAnyFlags( InSource->GetDesc().Usage, RHI::ERHIResourceUsage::CopySource ) or
         not HasAnyFlags( InDestination->GetDesc().Usage, RHI::ERHIResourceUsage::CopyDestination ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "CopyBuffer source/destination usage is incompatible." ) );
    }
    const TOptional<FOpenGLBuffer *> Source =
        RHI::TryUnwrapRHIBackend<FOpenGLBuffer>( StaticCastSharedRef<RHI::IRHIObject>( InSource ).Get() );
    const TOptional<FOpenGLBuffer *> Destination =
        RHI::TryUnwrapRHIBackend<FOpenGLBuffer>( StaticCastSharedRef<RHI::IRHIObject>( InDestination ).Get() );
    if ( not Source or not Destination )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "CopyBuffer requires OpenGL buffers on this device." ) );
    }
    const FOpenGLBufferCopyPayload Record{ .Source            = ( *Source )->GetGLBuffer(),
                                           .Destination       = ( *Destination )->GetGLBuffer(),
                                           .SourceOffset      = InCopy.SourceOffset + ( *Source )->GetGLBufferOffset(),
                                           .DestinationOffset = InCopy.DestinationOffset + ( *Destination )->GetGLBufferOffset(),
                                           .Size              = InCopy.Size };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::CopyBuffer ), Record ) );
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InSource ) );
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InDestination ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::CopyTexture ( TSharedRef<RHI::IRHITexture> InSource,
                                                                             TSharedRef<RHI::IRHITexture> InDestination,
                                                                             const RHI::FRHITextureCopy &InCopy ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    const RHI::ERHIFormat Format =
        InSource->GetDesc().Format != RHI::ERHIFormat::Unknown ? InSource->GetDesc().Format : InDestination->GetDesc().Format;
    LUMEN_TRY_EXPECT_ERROR( InCopy.Validate( Format ) );
    if ( not HasAnyFlags( InSource->GetDesc().Usage, RHI::ERHIResourceUsage::CopySource ) or
         not HasAnyFlags( InDestination->GetDesc().Usage, RHI::ERHIResourceUsage::CopyDestination ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "CopyTexture source/destination usage is incompatible." ) );
    }
    const TOptional<FOpenGLTexture *> Source =
        RHI::TryUnwrapRHIBackend<FOpenGLTexture>( StaticCastSharedRef<RHI::IRHIObject>( InSource ).Get() );
    const TOptional<FOpenGLTexture *> Destination =
        RHI::TryUnwrapRHIBackend<FOpenGLTexture>( StaticCastSharedRef<RHI::IRHIObject>( InDestination ).Get() );
    if ( not Source or not Destination )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "CopyTexture requires OpenGL textures on this device." ) );
    }
    const FOpenGLCopyTexturePayload Payload{
        .Source            = ( *Source )->GetTexture(),
        .Destination       = ( *Destination )->GetTexture(),
        .SourceTarget      = ( *Source )->GetTarget(),
        .DestinationTarget = ( *Destination )->GetTarget(),
        .SourceLevel       = static_cast<Int32>( InCopy.SourceMip ),
        .DestinationLevel  = static_cast<Int32>( InCopy.DestinationMip ),
        .SourceX           = InCopy.SourceOffset.X,
        .SourceY           = InCopy.SourceOffset.Y,
        .SourceZ           = InCopy.SourceOffset.Z + static_cast<Int32>( InCopy.SourceLayer ),
        .DestinationX      = InCopy.DestinationOffset.X,
        .DestinationY      = InCopy.DestinationOffset.Y,
        .DestinationZ      = InCopy.DestinationOffset.Z + static_cast<Int32>( InCopy.DestinationLayer ),
        .Width             = InCopy.Extent.Width,
        .Height            = InCopy.Extent.Height,
        .Depth             = InCopy.Extent.Depth,
    };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::CopyTexture ), Payload ) );
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InSource ) );
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InDestination ) );
    return {};
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::CopyBufferToTexture ( TSharedRef<RHI::IRHIBuffer> InSource,
                                                            TSharedRef<RHI::IRHITexture> InDestination,
                                                            const RHI::FRHIBufferTextureCopy &InCopy ) noexcept
{
    return RecordBufferTextureTransfer( InSource.Get(), InDestination.Get(), StaticCastSharedRef<RHI::IRHIObject>( InSource ),
                                        StaticCastSharedRef<RHI::IRHIObject>( InDestination ), InCopy, true );
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::CopyTextureToBuffer ( TSharedRef<RHI::IRHITexture> InSource,
                                                            TSharedRef<RHI::IRHIBuffer> InDestination,
                                                            const RHI::FRHIBufferTextureCopy &InCopy ) noexcept
{
    return RecordBufferTextureTransfer( InDestination.Get(), InSource.Get(),
                                        StaticCastSharedRef<RHI::IRHIObject>( InDestination ),
                                        StaticCastSharedRef<RHI::IRHIObject>( InSource ), InCopy, false );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::BeginRendering ( const RHI::FRHIRenderingDesc &InDesc ) noexcept
{
    if ( not SupportsRendering( GetQueueType() ) )
    {
        return TUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Command, "Rendering is not available on this queue." ) );
    }
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( false ) );
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    if ( InDesc.ColorAttachments.size() > 8U )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Rendering attachment count exceeds the OpenGL FBO cache capacity." ) );
    }

    FOpenGLRenderingPayload Payload;
    Payload.Area                 = InDesc.Area;
    Payload.ColorAttachmentCount = static_cast<UInt32>( InDesc.ColorAttachments.size() );
    Payload.bHasDepthStencil     = InDesc.DepthStencilAttachment.View.IsValid();
    for ( USize Index = 0U; Index < InDesc.ColorAttachments.size(); ++Index )
    {
        const RHI::FRHIRenderingAttachment &Attachment = InDesc.ColorAttachments[Index];
        const TOptional<FOpenGLTextureView *> View     = RHI::TryUnwrapRHIBackend<FOpenGLTextureView>( Attachment.View.Get() );
        if ( not View )
        {
            return TUnexpected( RHI::MakeRHIInvalidResource( "Rendering attachment is not an OpenGL texture view." ) );
        }
        RHI::ERHIFormat AttachmentFormat = Attachment.View->GetDesc().Format;
        if ( AttachmentFormat == RHI::ERHIFormat::Unknown and Attachment.View->GetTexture() )
        {
            AttachmentFormat = Attachment.View->GetTexture()->GetDesc().Format;
        }
        const TOptional<FOpenGLFormatMapping> Mapping = MapOpenGLFormat( AttachmentFormat );
        if ( not Mapping )
        {
            return TUnexpected(
                RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Resource, "Attachment format has no OpenGL mapping." ) );
        }
        Payload.ColorViews[Index]   = ( *View )->GetView();
        Payload.ColorFormats[Index] = Mapping->InternalFormat;
        Stream.Retain( StaticCastSharedPtr<RHI::IRHIObject>( Attachment.View ) );
    }
    if ( Payload.bHasDepthStencil )
    {
        const TOptional<FOpenGLTextureView *> View =
            RHI::TryUnwrapRHIBackend<FOpenGLTextureView>( InDesc.DepthStencilAttachment.View.Get() );
        if ( not View )
        {
            return TUnexpected( RHI::MakeRHIInvalidResource( "Depth-stencil attachment is not an OpenGL texture view." ) );
        }
        const TOptional<FOpenGLFormatMapping> Mapping = MapOpenGLFormat( InDesc.DepthStencilAttachment.View->GetDesc().Format );
        if ( not Mapping )
        {
            return TUnexpected(
                RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Resource, "Depth-stencil format has no OpenGL mapping." ) );
        }
        Payload.DepthView   = ( *View )->GetView();
        Payload.DepthFormat = Mapping->InternalFormat;
        Stream.Retain( StaticCastSharedPtr<RHI::IRHIObject>( InDesc.DepthStencilAttachment.View ) );
    }
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::BeginRendering ), Payload ) );
    State = RHI::ERHICommandListState::Rendering;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::EndRendering () noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRendering() );
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::EndRendering ), FOpenGLEmptyPayload{} ) );
    State = RHI::ERHICommandListState::Recording;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetPipeline ( TSharedRef<RHI::IRHIPipeline> InPipeline ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    GLuint Program     = 0U;
    GLuint VertexArray = 0U;
    Bool bCompute      = false;
    BoundGraphics      = nullptr;
    BoundLayout        = nullptr;
    BoundDynamicStates = RHI::ERHIDynamicState::None;
    if ( const TOptional<FOpenGLGraphicsPipeline *> Graphics =
             RHI::TryUnwrapRHIBackend<FOpenGLGraphicsPipeline>( InPipeline.Get() ) )
    {
        Program            = ( *Graphics )->GetProgram();
        VertexArray        = ( *Graphics )->GetVertexArray();
        BoundGraphics      = *Graphics;
        BoundLayout        = ( *Graphics )->GetNativeLayout();
        BoundDynamicStates = ( *Graphics )->GetDesc().DynamicStates;
    }
    else if ( const TOptional<FOpenGLComputePipeline *> Compute =
                  RHI::TryUnwrapRHIBackend<FOpenGLComputePipeline>( InPipeline.Get() ) )
    {
        Program     = ( *Compute )->GetProgram();
        BoundLayout = ( *Compute )->GetNativeLayout();
        bCompute    = true;
    }
    else
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Pipeline does not belong to the OpenGL backend." ) );
    }
    if ( bCompute and not SupportsCompute( GetQueueType() ) )
    {
        return TUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Pipeline, "Compute pipelines are not available on this queue." ) );
    }
    LUMEN_TRY_EXPECT_ERROR(
        Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::SetPipeline ),
                      FOpenGLSetPipelinePayload{ .Program = Program, .VertexArray = VertexArray, .bIsCompute = bCompute } ) );
    bHasPipeline      = true;
    BoundPipelineType = bCompute ? RHI::ERHIPipelineType::Compute : RHI::ERHIPipelineType::Graphics;
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InPipeline ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetBindingSet ( UInt32 InIndex,
                                                                               TSharedRef<RHI::IRHIBindingSet> InSet ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    if ( not bHasPipeline or BoundLayout == nullptr )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "SetBindingSet requires a bound pipeline layout." ) );
    }
    const TOptional<FOpenGLBindingSet *> Set = RHI::TryUnwrapRHIBackend<FOpenGLBindingSet>( InSet.Get() );
    if ( not Set )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Binding set does not belong to the OpenGL backend." ) );
    }
    if ( InIndex >= Context->GetLimits().MaxBindingSets )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Binding set index exceeds the device limit." ) );
    }
    const TOptional<FOpenGLBindingSetUnits> Units = BoundLayout->GetBindingSetUnits( InIndex );
    if ( not Units )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Pipeline layout has no units for this binding set." ) );
    }
    const TOptional<FOpenGLBindingLayout *> SetLayout =
        RHI::TryUnwrapRHIBackend<FOpenGLBindingLayout>( ( *Set )->GetLayout().Get() );
    if ( not SetLayout )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Binding layout does not belong to the OpenGL backend." ) );
    }

    FOpenGLBindingSetPayload Payload;
    Payload.SetIndex           = InIndex;
    Payload.UniformBufferBase  = Units->UniformBuffers.Base;
    Payload.StorageBufferBase  = Units->StorageBuffers.Base;
    Payload.TextureBase        = Units->Textures.Base;
    Payload.ImageBase          = Units->Images.Base;
    Payload.UniformBufferCount = Units->UniformBuffers.Count;
    Payload.StorageBufferCount = Units->StorageBuffers.Count;
    Payload.TextureCount       = Units->Textures.Count;
    Payload.ImageCount         = Units->Images.Count;

    for ( const RHI::FRHIBindingWrite &Write : ( *Set )->GetWrites() )
    {
        RHI::ERHIBindingType Type     = RHI::ERHIBindingType::ReadOnlyBuffer;
        UInt32 ClassIndex             = 0U;
        const Bool bResolved          = ResolveBindingClass( **SetLayout, Write.Slot, Write.ArrayElement, Type, ClassIndex );
        const FOpenGLUnitRange *Range = nullptr;
        UInt32 MaxClassIndex          = GOpenGLMaxDescriptorsPerSet;
        switch ( Type )
        {
        case RHI::ERHIBindingType::ConstantBuffer:
            Range = &Units->UniformBuffers;
            break;
        case RHI::ERHIBindingType::ReadOnlyBuffer:
        case RHI::ERHIBindingType::ReadWriteBuffer:
            Range = &Units->StorageBuffers;
            break;
        case RHI::ERHIBindingType::Texture:
        case RHI::ERHIBindingType::Sampler:
        case RHI::ERHIBindingType::CombinedTextureSampler:
            Range = &Units->Textures;
            break;
        case RHI::ERHIBindingType::StorageTexture:
            Range = &Units->Images;
            break;
        default:
            break;
        }
        if ( not bResolved or Range == nullptr or ClassIndex >= Range->Count or ClassIndex >= MaxClassIndex )
        {
            return TUnexpected( RHI::MakeRHIInvalidLimit( "Binding write does not fit its resolved unit range." ) );
        }
        const UInt32 Unit = Range->Base + ClassIndex;
        if ( const auto *BufferWrite = std::get_if<RHI::FRHIBufferBinding>( &Write.Resource ) )
        {
            const TOptional<FOpenGLBuffer *> Buffer = RHI::TryUnwrapRHIBackend<FOpenGLBuffer>( BufferWrite->Buffer.Get() );
            if ( not Buffer )
            {
                return TUnexpected( RHI::MakeRHIInvalidResource( "Bound buffer does not belong to this backend." ) );
            }
            if ( Type == RHI::ERHIBindingType::ConstantBuffer )
            {
                Payload.UniformBuffers[Unit] = ( *Buffer )->GetGLBuffer();
            }
            else
            {
                Payload.ShaderStorageBuffers[Unit] = ( *Buffer )->GetGLBuffer();
            }
            Stream.Retain( StaticCastSharedPtr<RHI::IRHIObject>( BufferWrite->Buffer ) );
        }
        else if ( const auto *Combined = std::get_if<RHI::FRHICombinedTextureSamplerBinding>( &Write.Resource ) )
        {
            const TOptional<FOpenGLTextureView *> View = RHI::TryUnwrapRHIBackend<FOpenGLTextureView>( Combined->View.Get() );
            const TOptional<FOpenGLSampler *> Sampler  = RHI::TryUnwrapRHIBackend<FOpenGLSampler>( Combined->Sampler.Get() );
            if ( not View or not Sampler )
            {
                return TUnexpected( RHI::MakeRHIInvalidResource( "Bound view/sampler does not belong to this backend." ) );
            }
            Payload.Textures[Unit] = ( *View )->GetView();
            Payload.Samplers[Unit] = ( *Sampler )->GetSampler();
            Stream.Retain( StaticCastSharedPtr<RHI::IRHIObject>( Combined->View ) );
            Stream.Retain( StaticCastSharedPtr<RHI::IRHIObject>( Combined->Sampler ) );
        }
        else if ( const auto *TextureWrite = std::get_if<RHI::FRHITextureBinding>( &Write.Resource ) )
        {
            const TOptional<FOpenGLTextureView *> View = RHI::TryUnwrapRHIBackend<FOpenGLTextureView>( TextureWrite->View.Get() );
            if ( not View )
            {
                return TUnexpected( RHI::MakeRHIInvalidResource( "Bound view does not belong to this backend." ) );
            }
            Payload.Textures[Unit] = ( *View )->GetView();
            Stream.Retain( StaticCastSharedPtr<RHI::IRHIObject>( TextureWrite->View ) );
        }
        else if ( const auto *SamplerWrite = std::get_if<RHI::FRHISamplerBinding>( &Write.Resource ) )
        {
            const TOptional<FOpenGLSampler *> Sampler = RHI::TryUnwrapRHIBackend<FOpenGLSampler>( SamplerWrite->Sampler.Get() );
            if ( not Sampler )
            {
                return TUnexpected( RHI::MakeRHIInvalidResource( "Bound sampler does not belong to this backend." ) );
            }
            Payload.Samplers[Unit] = ( *Sampler )->GetSampler();
            Stream.Retain( StaticCastSharedPtr<RHI::IRHIObject>( SamplerWrite->Sampler ) );
        }
    }
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::SetBindingSet ), Payload ) );
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InSet ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetInlineConstants ( const RHI::ERHIShaderStage InStages,
                                                                                    const UInt32 InOffset,
                                                                                    const TSpan<const Byte> InValues ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    if ( not bHasPipeline )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "SetInlineConstants requires a bound pipeline." ) );
    }
    LUMEN_TRY_EXPECT_ERROR( RHI::ValidateRHIInlineConstantWrite( InStages, InOffset, InValues.size() ) );
    if ( InOffset + InValues.size() > Context->GetLimits().MaxInlineConstantBytes or InValues.size() > 256U )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Inline constant write exceeds the layout range." ) );
    }
    FOpenGLInlineConstantPayload Payload;
    Payload.Offset = InOffset;
    Payload.Size   = static_cast<UInt32>( InValues.size() );
    for ( USize Index = 0U; Index < InValues.size(); ++Index )
    {
        Payload.Values[Index] = InValues[Index];
    }
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::SetInlineConstants ), Payload ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetVertexBuffer ( const UInt32 InSlot,
                                                                                 TSharedRef<RHI::IRHIBuffer> InBuffer,
                                                                                 const UInt64 InOffset ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    if ( InSlot >= Context->GetLimits().MaxVertexBuffers )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Vertex buffer slot exceeds the device limit." ) );
    }
    const UInt32 Stride = GetVertexBufferStride( InSlot );
    if ( Stride == 0U )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command,
                                                      "The bound vertex layout has no buffer for this slot." ) );
    }
    if ( not HasAnyFlags( InBuffer->GetDesc().Usage, RHI::ERHIResourceUsage::VertexBuffer ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Vertex buffer has incompatible usage." ) );
    }
    const TOptional<FOpenGLBuffer *> Native =
        RHI::TryUnwrapRHIBackend<FOpenGLBuffer>( StaticCastSharedRef<RHI::IRHIObject>( InBuffer ).Get() );
    if ( not Native )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Vertex buffer does not belong to this backend." ) );
    }
    const FOpenGLVertexBufferPayload Payload{ .Buffer = ( *Native )->GetGLBuffer(),
                                              .Offset = InOffset + ( *Native )->GetGLBufferOffset(),
                                              .Slot   = InSlot,
                                              .Stride = Stride };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::SetVertexBuffer ), Payload ) );
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InBuffer ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetIndexBuffer ( TSharedRef<RHI::IRHIBuffer> InBuffer,
                                                                                const UInt64 InOffset,
                                                                                const RHI::ERHIIndexFormat InFormat ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    if ( not HasAnyFlags( InBuffer->GetDesc().Usage, RHI::ERHIResourceUsage::IndexBuffer ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Index buffer has incompatible usage." ) );
    }
    const TOptional<FOpenGLBuffer *> Native =
        RHI::TryUnwrapRHIBackend<FOpenGLBuffer>( StaticCastSharedRef<RHI::IRHIObject>( InBuffer ).Get() );
    if ( not Native )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Index buffer does not belong to this backend." ) );
    }
    const GLenum IndexType = InFormat == RHI::ERHIIndexFormat::Index16 ? GlUnsignedShort : GlUnsignedInt;
    const FOpenGLIndexBufferPayload Payload{
        .Buffer = ( *Native )->GetGLBuffer(), .Offset = InOffset + ( *Native )->GetGLBufferOffset(), .IndexType = IndexType };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::SetIndexBuffer ), Payload ) );
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InBuffer ) );
    bHasIndexBuffer = true;
    BoundIndexType  = IndexType;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::Draw ( const RHI::FRHIDrawArguments &InArguments ) noexcept
{
    if ( not SupportsRendering( GetQueueType() ) )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Command, "Draw is not available on this queue." ) );
    }
    LUMEN_TRY_EXPECT_ERROR( RequireRendering() );
    LUMEN_TRY_EXPECT_ERROR( RequirePipeline( RHI::ERHIPipelineType::Graphics ) );
    LUMEN_TRY_EXPECT_ERROR( InArguments.Validate() );
    const FOpenGLDrawPayload Payload{ .VertexCount   = InArguments.VertexCount,
                                      .InstanceCount = InArguments.InstanceCount,
                                      .FirstVertex   = InArguments.FirstVertex,
                                      .FirstInstance = InArguments.FirstInstance };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::Draw ), Payload ) );
    return {};
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::DrawIndexed ( const RHI::FRHIDrawIndexedArguments &InArguments ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRendering() );
    LUMEN_TRY_EXPECT_ERROR( RequirePipeline( RHI::ERHIPipelineType::Graphics ) );
    LUMEN_TRY_EXPECT_ERROR( InArguments.Validate() );
    if ( not bHasIndexBuffer )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "DrawIndexed requires an index buffer." ) );
    }
    const FOpenGLDrawIndexedPayload Payload{ .IndexCount    = InArguments.IndexCount,
                                             .InstanceCount = InArguments.InstanceCount,
                                             .FirstIndex    = InArguments.FirstIndex,
                                             .VertexOffset  = InArguments.VertexOffset,
                                             .FirstInstance = InArguments.FirstInstance,
                                             .IndexType     = BoundIndexType };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::DrawIndexed ), Payload ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::DrawIndirect ( TSharedRef<RHI::IRHIBuffer> InArguments,
                                                                              const RHI::FRHIIndirectArguments &InDesc ) noexcept
{
    if ( not Context->GetCapabilities().Supports( RHI::ERHICapabilityFlags::IndirectDraw ) )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Command, "Indirect draws are unavailable." ) );
    }
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    LUMEN_TRY_EXPECT_ERROR( RequirePipeline( RHI::ERHIPipelineType::Graphics ) );
    if ( not HasAnyFlags( InArguments->GetDesc().Usage, RHI::ERHIResourceUsage::IndirectArgument ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Indirect arguments buffer has the wrong usage." ) );
    }
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate( InArguments->GetDesc().Size, 16U ) );
    const TOptional<FOpenGLBuffer *> Native =
        RHI::TryUnwrapRHIBackend<FOpenGLBuffer>( StaticCastSharedRef<RHI::IRHIObject>( InArguments ).Get() );
    if ( not Native )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Indirect arguments buffer is not an OpenGL buffer." ) );
    }
    const FOpenGLIndirectPayload Payload{ .Buffer    = ( *Native )->GetGLBuffer(),
                                          .Offset    = InDesc.Offset + ( *Native )->GetGLBufferOffset(),
                                          .Count     = InDesc.Count,
                                          .Stride    = InDesc.Stride,
                                          .bIndexed  = false,
                                          .IndexType = GlUnsignedInt };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::DrawIndirect ), Payload ) );
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InArguments ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::Dispatch ( const RHI::FRHIDispatchArguments &InArguments ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( false ) );
    LUMEN_TRY_EXPECT_ERROR( RequirePipeline( RHI::ERHIPipelineType::Compute ) );
    LUMEN_TRY_EXPECT_ERROR( InArguments.Validate( Context->GetLimits().MaxComputeWorkgroupSize ) );
    const FOpenGLDispatchPayload Payload{
        .X = InArguments.GroupCountX, .Y = InArguments.GroupCountY, .Z = InArguments.GroupCountZ };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::Dispatch ), Payload ) );
    return {};
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::DispatchIndirect ( TSharedRef<RHI::IRHIBuffer> InArguments,
                                                         const RHI::FRHIIndirectArguments &InDesc ) noexcept
{
    if ( not Context->GetCapabilities().Supports( RHI::ERHICapabilityFlags::IndirectDraw ) )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Command, "Indirect dispatch is unavailable." ) );
    }
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( false ) );
    LUMEN_TRY_EXPECT_ERROR( RequirePipeline( RHI::ERHIPipelineType::Compute ) );
    if ( not HasAnyFlags( InArguments->GetDesc().Usage, RHI::ERHIResourceUsage::IndirectArgument ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Indirect arguments buffer has the wrong usage." ) );
    }
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate( InArguments->GetDesc().Size, sizeof( RHI::FRHIDispatchArguments ) ) );
    const TOptional<FOpenGLBuffer *> Buffer =
        RHI::TryUnwrapRHIBackend<FOpenGLBuffer>( StaticCastSharedRef<RHI::IRHIObject>( InArguments ).Get() );
    if ( not Buffer )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Indirect arguments buffer is not an OpenGL buffer." ) );
    }
    const FOpenGLIndirectPayload Payload{ .Buffer   = ( *Buffer )->GetGLBuffer(),
                                          .Offset   = InDesc.Offset + ( *Buffer )->GetGLBufferOffset(),
                                          .Count    = InDesc.Count,
                                          .Stride   = InDesc.Stride,
                                          .bIndexed = false };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::DispatchIndirect ), Payload ) );
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InArguments ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetViewport ( const RHI::FRHIViewport &InViewport ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    if ( not InViewport.IsValid() )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Command, "Viewport is invalid." ) );
    }
    const FOpenGLViewportPayload Payload{ .X        = InViewport.X,
                                          .Y        = InViewport.Y,
                                          .Width    = InViewport.Width,
                                          .Height   = InViewport.Height,
                                          .MinDepth = InViewport.MinDepth,
                                          .MaxDepth = InViewport.MaxDepth };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::SetViewport ), Payload ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetScissor ( const RHI::FRHIRect2D &InScissor ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    if ( not InScissor.IsValid() )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Command, "Scissor extent is empty." ) );
    }
    const FOpenGLScissorPayload Payload{
        .X = InScissor.Position.X, .Y = InScissor.Position.Y, .Width = InScissor.Size.Width, .Height = InScissor.Size.Height };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::SetScissor ), Payload ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    LUMEN_TRY_EXPECT_ERROR( RequireDynamicState( RHI::ERHIDynamicState::BlendConstants ) );
    const FOpenGLFloat4Payload Payload{ .Values = { InConstants[0], InConstants[1], InConstants[2], InConstants[3] } };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::SetBlendConstants ), Payload ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetDepthBias ( const Float32 InConstant,
                                                                              const Float32 InSlope ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    LUMEN_TRY_EXPECT_ERROR( RequireDynamicState( RHI::ERHIDynamicState::DepthBias ) );
    const FOpenGLDepthBiasPayload Payload{ .Constant = InConstant, .Slope = InSlope };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::SetDepthBias ), Payload ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetStencilReference ( const UInt32 InReference ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    LUMEN_TRY_EXPECT_ERROR( RequireDynamicState( RHI::ERHIDynamicState::StencilReference ) );
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::SetStencilReference ),
                                          FOpenGLUIntPayload{ .Value = InReference } ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::BeginQuery ( TSharedRef<RHI::IRHIQueryPool> InPool,
                                                                            const UInt32 InIndex ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    if ( ActiveQueryPool != nullptr )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Query, "A query is already active." ) );
    }
    const TOptional<FOpenGLQueryPool *> Pool = RHI::TryUnwrapRHIBackend<FOpenGLQueryPool>( InPool.Get() );
    if ( not Pool or ( *Pool )->GetDesc().QueueType != GetQueueType() )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Query pool is not owned by this queue." ) );
    }
    LUMEN_TRY_EXPECT_ERROR( ( *Pool )->Begin( InIndex ) );
    const FOpenGLQueryPayload Payload{ .Pool       = *Pool,
                                       .Query      = ( *Pool )->GetQuery( InIndex ),
                                       .Index      = InIndex,
                                       .PoolIndex  = RegisterQueryPool( QueryPools, InPool ),
                                       .Generation = ( *Pool )->GetGeneration( InIndex ),
                                       .Target     = ( *Pool )->GetNativeTarget() };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::BeginQuery ), Payload ) );
    ActiveQueryPool  = InPool.Get();
    ActiveQueryIndex = InIndex;
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InPool ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::EndQuery ( TSharedRef<RHI::IRHIQueryPool> InPool,
                                                                          const UInt32 InIndex ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    if ( ActiveQueryPool != InPool.Get() or ActiveQueryIndex != InIndex )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Query, "No matching query is active." ) );
    }
    const TOptional<FOpenGLQueryPool *> Pool = RHI::TryUnwrapRHIBackend<FOpenGLQueryPool>( InPool.Get() );
    if ( not Pool )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Query pool is not an OpenGL pool." ) );
    }
    LUMEN_TRY_EXPECT_ERROR( ( *Pool )->End( InIndex ) );
    const FOpenGLQueryPayload Payload{ .Pool       = *Pool,
                                       .Query      = ( *Pool )->GetQuery( InIndex ),
                                       .Index      = InIndex,
                                       .PoolIndex  = RegisterQueryPool( QueryPools, InPool ),
                                       .Generation = ( *Pool )->GetGeneration( InIndex ),
                                       .Target     = ( *Pool )->GetNativeTarget() };
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::EndQuery ), Payload ) );
    ActiveQueryPool  = nullptr;
    ActiveQueryIndex = 0U;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::BeginDebugRegion ( const FStringView InName ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    if ( InName.empty() )
    {
        return InvalidCommand( "Debug region name is empty." );
    }
    ++DebugDepth;
    FOpenGLDebugRegionPayload Payload;
    Payload.Length = static_cast<UInt32>( InName.size() );
    TVector<Byte> Record( sizeof( FOpenGLDebugRegionPayload ) + InName.size() );
    const Byte *PayloadBytes = reinterpret_cast<const Byte *>( &Payload );
    for ( USize Index = 0U; Index < sizeof( FOpenGLDebugRegionPayload ); ++Index )
    {
        Record[Index] = PayloadBytes[Index];
    }
    for ( USize Index = 0U; Index < InName.size(); ++Index )
    {
        Record[sizeof( FOpenGLDebugRegionPayload ) + Index] = static_cast<Byte>( InName[Index] );
    }
    return Stream.WritePayload( static_cast<UInt32>( EOpenGLCommandOpcode::BeginDebugRegion ), Record );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::EndDebugRegion () noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    if ( DebugDepth == 0U )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "No debug region is open." ) );
    }
    --DebugDepth;
    return Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::EndDebugRegion ), FOpenGLUIntPayload{} );
}

/**
 * Private
 */

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::RequireRecording ( const Bool bAllowRendering ) const noexcept
{
    const Bool bValid =
        State == RHI::ERHICommandListState::Recording or ( bAllowRendering and State == RHI::ERHICommandListState::Rendering );
    if ( not bValid )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "Command list is not recording." ) );
    }
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::RequireRendering () const noexcept
{
    if ( State != RHI::ERHICommandListState::Rendering )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "No rendering scope is open." ) );
    }
    return {};
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::RequirePipeline ( const RHI::ERHIPipelineType InType ) const noexcept
{
    if ( not bHasPipeline or BoundPipelineType != InType )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Pipeline, "The bound pipeline does not match the command." ) );
    }
    return {};
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::UnsupportedCommand ( const FStringView InOperation,
                                                           const RHI::ERHIErrorCondition InCondition ) noexcept
{
    return TUnexpected( RHI::MakeRHIUnsupported( InCondition, InOperation ) );
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::RequireDynamicState ( const RHI::ERHIDynamicState InState ) const noexcept
{
    if ( not HasAnyFlags( BoundDynamicStates, InState ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command,
                                                      "The bound graphics pipeline does not declare this dynamic state." ) );
    }
    return {};
}

Lumen::UInt32 Lumen::RHIOpenGL::FOpenGLCommandList::GetVertexBufferStride ( const UInt32 InSlot ) const noexcept
{
    if ( BoundGraphics == nullptr )
    {
        return 0U;
    }
    const RHI::FRHIVertexBufferLayout *Found = Algo::FindIf(
        BoundGraphics->GetDesc().VertexLayout.Buffers, [InSlot] ( const RHI::FRHIVertexBufferLayout &InLayout ) noexcept -> Bool
    {
        return InLayout.Slot == InSlot;
    } );
    return ( Found != nullptr ) ? Found->Stride : 0U;
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::RecordBufferTextureTransfer ( RHI::IRHIBuffer *InBuffer,
                                                                    RHI::IRHITexture *InTexture,
                                                                    TSharedPtr<RHI::IRHIObject> InBufferRef,
                                                                    TSharedPtr<RHI::IRHIObject> InTextureRef,
                                                                    const RHI::FRHIBufferTextureCopy &InCopy,
                                                                    const Bool bFromBuffer ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    if ( InBuffer == nullptr or InTexture == nullptr )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Buffer/texture copy requires valid resources." ) );
    }
    const RHI::ERHIFormat Format = InTexture->GetDesc().Format;
    LUMEN_TRY_EXPECT_ERROR( InCopy.Validate( Format ) );
    const RHI::FRHIFormatInfo &FormatInfo = RHI::GetRHIFormatInfo( Format );
    if ( FormatInfo.bIsCompressed or FormatInfo.BlockWidth == 0U or FormatInfo.BytesPerBlock == 0U )
    {
        return TUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Command, "Compressed buffer/texture transfers are unavailable." ) );
    }
    const UInt32 BytesPerPixel = static_cast<UInt32>( FormatInfo.BytesPerBlock ) / static_cast<UInt32>( FormatInfo.BlockWidth );
    if ( BytesPerPixel == 0U or ( InCopy.RowPitch != 0ULL and ( InCopy.RowPitch % BytesPerPixel ) != 0ULL ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Command, "Invalid transfer row pitch." ) );
    }
    const UInt64 RowLength64 = InCopy.RowPitch != 0ULL ? InCopy.RowPitch / BytesPerPixel : 0ULL;
    if ( RowLength64 > 0xFFFFFFFFULL )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Transfer row length exceeds the OpenGL limit." ) );
    }
    if ( bFromBuffer and not HasAnyFlags( InBuffer->GetDesc().Usage, RHI::ERHIResourceUsage::CopySource ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Transfer buffer is not a copy source." ) );
    }
    if ( ( not bFromBuffer ) and not HasAnyFlags( InBuffer->GetDesc().Usage, RHI::ERHIResourceUsage::CopyDestination ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Transfer buffer is not a copy destination." ) );
    }
    if ( bFromBuffer and not HasAnyFlags( InTexture->GetDesc().Usage, RHI::ERHIResourceUsage::CopyDestination ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Transfer texture is not a copy destination." ) );
    }
    if ( ( not bFromBuffer ) and not HasAnyFlags( InTexture->GetDesc().Usage, RHI::ERHIResourceUsage::CopySource ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Transfer texture is not a copy source." ) );
    }
    const TOptional<FOpenGLBuffer *> NativeBuffer =
        RHI::TryUnwrapRHIBackend<FOpenGLBuffer>( static_cast<RHI::IRHIObject *>( InBuffer ) );
    const TOptional<FOpenGLTexture *> NativeTexture =
        RHI::TryUnwrapRHIBackend<FOpenGLTexture>( static_cast<RHI::IRHIObject *>( InTexture ) );
    const TOptional<FOpenGLFormatMapping> Mapping = MapOpenGLFormat( Format );
    if ( not NativeBuffer or not NativeTexture or not Mapping )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Transfer resources require an OpenGL mapping." ) );
    }
    const FOpenGLBufferTextureCopyPayload Payload{
        .Buffer        = ( *NativeBuffer )->GetGLBuffer(),
        .BufferOffset  = InCopy.BufferOffset + ( *NativeBuffer )->GetGLBufferOffset(),
        .Texture       = ( *NativeTexture )->GetTexture(),
        .TextureTarget = ( *NativeTexture )->GetTarget(),
        .Level         = static_cast<Int32>( InCopy.Mip ),
        .X             = InCopy.TextureOffset.X,
        .Y             = InCopy.TextureOffset.Y,
        .Z             = InCopy.TextureOffset.Z + static_cast<Int32>( InCopy.Layer ),
        .Width         = InCopy.Extent.Width,
        .Height        = InCopy.Extent.Height,
        .Depth         = InCopy.Extent.Depth,
        .RowLength     = static_cast<UInt32>( RowLength64 ),
        .BytesPerPixel = BytesPerPixel,
        .Format        = Mapping->ExternalFormat,
        .Type          = Mapping->Type,
        .bFromBuffer   = bFromBuffer,
    };
    const EOpenGLCommandOpcode Opcode =
        bFromBuffer ? EOpenGLCommandOpcode::CopyBufferToTexture : EOpenGLCommandOpcode::CopyTextureToBuffer;
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( Opcode ), Payload ) );
    Stream.Retain( static_cast<TSharedPtr<RHI::IRHIObject> &&>( InBufferRef ) );
    Stream.Retain( static_cast<TSharedPtr<RHI::IRHIObject> &&>( InTextureRef ) );
    return {};
}
