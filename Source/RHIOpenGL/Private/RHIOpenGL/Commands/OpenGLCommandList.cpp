/**
 * @file OpenGLCommandList.cpp
 * @brief Implements validated command recording for replay on the GL Executor.
 */

#include <RHIOpenGL/Commands/OpenGLCommandList.hpp>

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
#include <RHI/Types/RHIHash.hpp>

#include <RHI/Validation/RHIValidation.hpp>
#include <cstring>

#include <RHIOpenGL/Bindings/OpenGLBindingSet.hpp>
#include <RHIOpenGL/Commands/OpenGLCommandStreamOpcodes.hpp>
#include <RHIOpenGL/Pipelines/OpenGLComputePipeline.hpp>
#include <RHIOpenGL/Pipelines/OpenGLGraphicsPipeline.hpp>
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

using namespace Lumen;
using namespace Lumen::RHIOpenGL;
using namespace Lumen::RHI;

[[nodiscard]] FRHIExpected InvalidCommand ( FStringView InMessage ) noexcept
{
    return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, InMessage ) );
}

[[nodiscard]] Bool SupportsRendering ( ERHIQueueType InQueueType ) noexcept
{
    return InQueueType == ERHIQueueType::Graphics;
}

[[nodiscard]] Bool SupportsCompute ( ERHIQueueType InQueueType ) noexcept
{
    return InQueueType == ERHIQueueType::Graphics or InQueueType == ERHIQueueType::Compute;
}

[[nodiscard]] GLbitfield BarrierBitsForState ( ERHIResourceState InState ) noexcept
{
    switch ( InState )
    {
    case ERHIResourceState::CopySource:
        return GlShaderStorageBarrierBit | GlBufferUpdateBarrierBit;
    case ERHIResourceState::CopyDestination:
        return GlBufferUpdateBarrierBit | GlFramebufferBarrierBit;
    case ERHIResourceState::ShaderResource:
        return GlTextureFetchBarrierBit | GlShaderStorageBarrierBit | GlUniformBarrierBit;
    case ERHIResourceState::UnorderedAccess:
        return GlShaderStorageBarrierBit | GlShaderImageAccessBarrierBit;
    case ERHIResourceState::RenderTarget:
        return GlFramebufferBarrierBit | GlColorBufferBit;
    case ERHIResourceState::VertexBuffer:
        return GlVertexAttribArrayBarrierBit;
    case ERHIResourceState::IndexBuffer:
        return GlElementArrayBarrierBit;
    case ERHIResourceState::IndirectArgument:
        return GlCommandBarrierBit;
    default:
        return GlClientMappedBufferBarrierBit;
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

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::Begin () noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RHI::RHITransitionCommandList( State, RHI::ERHICommandListState::Recording ) );
    Stream.Clear();
    bHasPipeline     = false;
    ActiveQueryPool  = nullptr;
    ActiveQueryIndex = 0U;
    DebugDepth       = 0U;
    Completion       = {};
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
    State            = RHI::ERHICommandListState::Initial;
    Completion       = {};
    bHasPipeline     = false;
    ActiveQueryPool  = nullptr;
    ActiveQueryIndex = 0U;
    DebugDepth       = 0U;
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
        BarrierBits |= BufferTransition ? BarrierBitsForState( BufferTransition->After ) : GlFramebufferBarrierBit;
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

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::CopyTexture ( TSharedRef<RHI::IRHITexture>,
                                                                             TSharedRef<RHI::IRHITexture>,
                                                                             const RHI::FRHITextureCopy & ) noexcept
{
    return UnsupportedCommand( "CopyTexture", RHI::ERHIErrorCondition::Command );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::CopyBufferToTexture ( TSharedRef<RHI::IRHIBuffer>,
                                                                                     TSharedRef<RHI::IRHITexture>,
                                                                                     const RHI::FRHIBufferTextureCopy & ) noexcept
{
    return UnsupportedCommand( "CopyBufferToTexture", RHI::ERHIErrorCondition::Command );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::CopyTextureToBuffer ( TSharedRef<RHI::IRHITexture>,
                                                                                     TSharedRef<RHI::IRHIBuffer>,
                                                                                     const RHI::FRHIBufferTextureCopy & ) noexcept
{
    return UnsupportedCommand( "CopyTextureToBuffer", RHI::ERHIErrorCondition::Command );
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
        ERHIFormat AttachmentFormat = Attachment.View->GetDesc().Format;
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
    GLuint Program = 0U;
    Bool bCompute  = false;
    if ( const TOptional<FOpenGLGraphicsPipeline *> Graphics =
             RHI::TryUnwrapRHIBackend<FOpenGLGraphicsPipeline>( InPipeline.Get() ) )
    {
        Program = ( *Graphics )->GetProgram();
    }
    else if ( const TOptional<FOpenGLComputePipeline *> Compute =
                  RHI::TryUnwrapRHIBackend<FOpenGLComputePipeline>( InPipeline.Get() ) )
    {
        Program  = ( *Compute )->GetProgram();
        bCompute = true;
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
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::SetPipeline ),
                                          FOpenGLSetPipelinePayload{ .Program = Program, .bIsCompute = bCompute } ) );
    bHasPipeline      = true;
    BoundPipelineType = bCompute ? RHI::ERHIPipelineType::Compute : RHI::ERHIPipelineType::Graphics;
    Stream.Retain( StaticCastSharedRef<RHI::IRHIObject>( InPipeline ) );
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetBindingSet ( UInt32 InIndex,
                                                                               TSharedRef<RHI::IRHIBindingSet> InSet ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RequireRecording( true ) );
    const TOptional<FOpenGLBindingSet *> Set = RHI::TryUnwrapRHIBackend<FOpenGLBindingSet>( InSet.Get() );
    if ( not Set )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Binding set does not belong to the OpenGL backend." ) );
    }
    if ( InIndex >= Context->GetLimits().MaxBindingSets )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Binding set index exceeds the device limit." ) );
    }
    FOpenGLBindingSetPayload Payload;
    for ( const RHI::FRHIBindingWrite &Write : ( *Set )->GetWrites() )
    {
        if ( Payload.UnitCount >= 8U )
        {
            break;
        }
        const UInt32 Unit = Payload.UnitCount++;
        if ( const auto *BufferWrite = std::get_if<RHI::FRHIBufferBinding>( &Write.Resource ) )
        {
            const TOptional<FOpenGLBuffer *> Buffer = RHI::TryUnwrapRHIBackend<FOpenGLBuffer>( BufferWrite->Buffer.Get() );
            if ( Buffer )
            {
                Payload.UniformBuffers[Unit] = ( *Buffer )->GetGLBuffer();
            }
        }
        else if ( const auto *Combined = std::get_if<RHI::FRHICombinedTextureSamplerBinding>( &Write.Resource ) )
        {
            const TOptional<FOpenGLTextureView *> View = RHI::TryUnwrapRHIBackend<FOpenGLTextureView>( Combined->View.Get() );
            const TOptional<FOpenGLSampler *> Sampler  = RHI::TryUnwrapRHIBackend<FOpenGLSampler>( Combined->Sampler.Get() );
            if ( View )
            {
                Payload.Textures[Unit] = ( *View )->GetView();
            }
            if ( Sampler )
            {
                Payload.Samplers[Unit] = ( *Sampler )->GetSampler();
            }
        }
        else if ( const auto *TextureWrite = std::get_if<RHI::FRHITextureBinding>( &Write.Resource ) )
        {
            const TOptional<FOpenGLTextureView *> View = RHI::TryUnwrapRHIBackend<FOpenGLTextureView>( TextureWrite->View.Get() );
            if ( View )
            {
                Payload.Textures[Unit] = ( *View )->GetView();
            }
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

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::SetVertexBuffer ( UInt32, TSharedRef<RHI::IRHIBuffer>, UInt64 ) noexcept
{
    return UnsupportedCommand( "SetVertexBuffer", RHI::ERHIErrorCondition::Command );
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::SetIndexBuffer ( TSharedRef<RHI::IRHIBuffer>, UInt64, RHI::ERHIIndexFormat ) noexcept
{
    return UnsupportedCommand( "SetIndexBuffer", RHI::ERHIErrorCondition::Command );
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

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::DrawIndexed ( const RHI::FRHIDrawIndexedArguments & ) noexcept
{
    return UnsupportedCommand( "DrawIndexed", RHI::ERHIErrorCondition::Command );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::DrawIndirect ( TSharedRef<RHI::IRHIBuffer>,
                                                                              const RHI::FRHIIndirectArguments & ) noexcept
{
    return UnsupportedCommand( "DrawIndirect", RHI::ERHIErrorCondition::Command );
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

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetViewport ( const RHI::FRHIViewport & ) noexcept
{
    return UnsupportedCommand( "SetViewport", RHI::ERHIErrorCondition::Command );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetScissor ( const RHI::FRHIRect2D & ) noexcept
{
    return UnsupportedCommand( "SetScissor", RHI::ERHIErrorCondition::Command );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetBlendConstants ( const Float32 ( & )[4] ) noexcept
{
    return UnsupportedCommand( "SetBlendConstants", RHI::ERHIErrorCondition::Command );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetDepthBias ( Float32, Float32 ) noexcept
{
    return UnsupportedCommand( "SetDepthBias", RHI::ERHIErrorCondition::Command );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetStencilReference ( UInt32 ) noexcept
{
    return UnsupportedCommand( "SetStencilReference", RHI::ERHIErrorCondition::Command );
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
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::BeginQuery ),
                                          FOpenGLQueryPayload{ .Query = ( *Pool )->GetQuery( InIndex ), .Index = InIndex } ) );
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
    LUMEN_TRY_EXPECT_ERROR( Stream.Write( static_cast<UInt32>( EOpenGLCommandOpcode::EndQuery ),
                                          FOpenGLQueryPayload{ .Query = ( *Pool )->GetQuery( InIndex ), .Index = InIndex } ) );
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
