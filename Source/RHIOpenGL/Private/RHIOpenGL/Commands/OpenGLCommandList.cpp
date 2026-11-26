/**
 * @file OpenGLCommandList.cpp
 * @brief Implements portable command recording and OpenGL buffer-copy capture.
 */

#include <RHIOpenGL/Commands/OpenGLCommandList.hpp>

#include <RHI/Commands/RHICommandListState.hpp>
#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Resources/OpenGLBuffer.hpp>

/**
 * File-local helpers
 */

namespace
{

constexpr Lumen::UInt32 OpenGLBufferCopyOpcode = 1U;

struct FRecordedBufferCopy
{
    const Lumen::RHIOpenGL::FOpenGLBuffer *Source      = nullptr;
    const Lumen::RHIOpenGL::FOpenGLBuffer *Destination = nullptr;
    Lumen::UInt64 SourceOffset                         = 0ULL;
    Lumen::UInt64 DestinationOffset                    = 0ULL;
    Lumen::UInt64 Size                                 = 0ULL;
};

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
    return State;
}

Lumen::RHI::FRHITimelineValue Lumen::RHIOpenGL::FOpenGLCommandList::GetCompletion () const noexcept
{
    return Completion;
}

const Lumen::RHI::FRHICommandStream &Lumen::RHIOpenGL::FOpenGLCommandList::GetStream () const noexcept
{
    return Stream;
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::Begin () noexcept
{
    if ( const RHI::FRHIExpected Transitioned = RHI::RHITransitionCommandList( State, RHI::ERHICommandListState::Recording );
         not Transitioned )
    {
        return TUnexpected( Transitioned.Error() );
    }
    Stream.Clear();
    bRendering = false;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::End () noexcept
{
    if ( bRendering )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "Rendering scope is still open." ) );
    }
    if ( const RHI::FRHIExpected Transitioned = RHI::RHITransitionCommandList( State, RHI::ERHICommandListState::Executable );
         not Transitioned )
    {
        return TUnexpected( Transitioned.Error() );
    }
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::Reset () noexcept
{
    if ( State == RHI::ERHICommandListState::Pending or bRendering )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "Command list cannot be reset." ) );
    }
    Stream.Clear();
    State      = RHI::ERHICommandListState::Initial;
    Completion = {};
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::MarkPending ( const RHI::FRHITimelineValue InCompletion ) noexcept
{
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
    if ( State != RHI::ERHICommandListState::Pending or not RHI::IsRHICompletionReached( InCompleted, Completion ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "Completion was not reached." ) );
    }
    State      = RHI::ERHICommandListState::Executable;
    Completion = {};
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::Transition ( const TSpan<const RHI::FRHITransition> ) noexcept
{
    return UnsupportedCommand( "Transition", RHI::ERHIErrorCondition::Command );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::CopyBuffer ( TSharedRef<RHI::IRHIBuffer> InSource,
                                                                            TSharedRef<RHI::IRHIBuffer> InDestination,
                                                                            const RHI::FRHIBufferCopy &InCopy ) noexcept
{
    if ( const RHI::FRHIExpected Recording = RequireRecording(); not Recording )
    {
        return TUnexpected( Recording.Error() );
    }
    if ( const RHI::FRHIExpected Validated = InCopy.Validate(); not Validated )
    {
        return TUnexpected( Validated.Error() );
    }
    if ( not HasAnyFlags( InSource->GetDesc().Usage, RHI::ERHIResourceUsage::CopySource ) or
         not HasAnyFlags( InDestination->GetDesc().Usage, RHI::ERHIResourceUsage::CopyDestination ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "CopyBuffer source/destination usage is incompatible." ) );
    }
    const TOptional<FOpenGLBuffer *> Source      = RHI::TryRHIBackendCast<FOpenGLBuffer>( *InSource );
    const TOptional<FOpenGLBuffer *> Destination = RHI::TryRHIBackendCast<FOpenGLBuffer>( *InDestination );
    if ( not Source or not Destination )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "CopyBuffer requires OpenGL buffers on this device." ) );
    }
    if ( InCopy.SourceOffset + InCopy.Size > InSource->GetDesc().Size or
         InCopy.DestinationOffset + InCopy.Size > InDestination->GetDesc().Size )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "CopyBuffer range is out of bounds." ) );
    }
    const FRecordedBufferCopy Record{ .Source            = *Source,
                                      .Destination       = *Destination,
                                      .SourceOffset      = InCopy.SourceOffset,
                                      .DestinationOffset = InCopy.DestinationOffset,
                                      .Size              = InCopy.Size };
    if ( const RHI::FRHIExpected Written = Stream.Write( OpenGLBufferCopyOpcode, Record ); not Written )
    {
        return TUnexpected( Written.Error() );
    }
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

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::BeginRendering ( const RHI::FRHIRenderingDesc & ) noexcept
{
    if ( const RHI::FRHIExpected Recording = RequireRecording(); not Recording )
    {
        return TUnexpected( Recording.Error() );
    }
    bRendering = true;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::EndRendering () noexcept
{
    if ( not bRendering )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "No rendering scope is open." ) );
    }
    bRendering = false;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetPipeline ( TSharedRef<RHI::IRHIPipeline> ) noexcept
{
    return UnsupportedCommand( "SetPipeline", RHI::ERHIErrorCondition::Pipeline );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::SetBindingSet ( UInt32, TSharedRef<RHI::IRHIBindingSet> ) noexcept
{
    return UnsupportedCommand( "SetBindingSet", RHI::ERHIErrorCondition::Binding );
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::SetInlineConstants ( RHI::ERHIShaderStage, UInt32, TSpan<const Byte> ) noexcept
{
    return UnsupportedCommand( "SetInlineConstants", RHI::ERHIErrorCondition::Command );
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

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::Draw ( const RHI::FRHIDrawArguments & ) noexcept
{
    return UnsupportedCommand( "Draw", RHI::ERHIErrorCondition::Command );
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

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::Dispatch ( const RHI::FRHIDispatchArguments & ) noexcept
{
    return UnsupportedCommand( "Dispatch", RHI::ERHIErrorCondition::Command );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::DispatchIndirect ( TSharedRef<RHI::IRHIBuffer>,
                                                                                  const RHI::FRHIIndirectArguments & ) noexcept
{
    return UnsupportedCommand( "DispatchIndirect", RHI::ERHIErrorCondition::Command );
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

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::BeginQuery ( TSharedRef<RHI::IRHIQueryPool>, UInt32 ) noexcept
{
    return UnsupportedCommand( "BeginQuery", RHI::ERHIErrorCondition::Query );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::EndQuery ( TSharedRef<RHI::IRHIQueryPool>, UInt32 ) noexcept
{
    return UnsupportedCommand( "EndQuery", RHI::ERHIErrorCondition::Query );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::BeginDebugRegion ( FStringView ) noexcept
{
    return UnsupportedCommand( "BeginDebugRegion", RHI::ERHIErrorCondition::Command );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::EndDebugRegion () noexcept
{
    return UnsupportedCommand( "EndDebugRegion", RHI::ERHIErrorCondition::Command );
}

/**
 * Private
 */

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLCommandList::RequireRecording () const noexcept
{
    if ( State != RHI::ERHICommandListState::Recording )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Command, "Command list is not recording." ) );
    }
    return {};
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandList::UnsupportedCommand ( const FStringView InOperation,
                                                           const RHI::ERHIErrorCondition InCondition ) noexcept
{
    return TUnexpected( RHI::MakeRHIUnsupported( InCondition, InOperation ) );
}
