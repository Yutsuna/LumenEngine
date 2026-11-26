/**
 * @file OpenGLCommandExecutor.cpp
 * @brief Implements stream decoding and single-threaded OpenGL command replay.
 */

#include <RHIOpenGL/Commands/OpenGLCommandExecutor.hpp>

#include <RHI/Debug/RHIDebugMessage.hpp>
#include <RHI/Pipelines/RHIInlineConstantRange.hpp>
#include <RHI/Validation/RHIValidation.hpp>

#include <RHIOpenGL/Pipelines/OpenGLComputePipeline.hpp>
#include <RHIOpenGL/Pipelines/OpenGLGraphicsPipeline.hpp>
#include <RHIOpenGL/Synchronization/OpenGLQueryPool.hpp>

#include <Algo/Any.hpp>
#include <Memory/Span.hpp>
#include <String/StringView.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Unexpected.hpp>

#include <cstring>

/**
 * File-local helpers
 */

namespace
{

constexpr Lumen::RHIOpenGL::GLenum GlColorBuffer        = 0x1800U;
constexpr Lumen::RHIOpenGL::GLenum GlDepthStencilBuffer = 0x1881U;
constexpr Lumen::RHIOpenGL::GLenum GlUnpackRowLength    = 0x0CF2U;

/** @brief Replay-local state that changes only while walking one stream. */
struct FReplayState
{
    const Lumen::RHIOpenGL::FGLDispatchTable &Dispatch;
    Lumen::RHIOpenGL::FOpenGLDeviceContext &Context;
    Lumen::RHIOpenGL::FOpenGLStateCache &State;
    Lumen::RHIOpenGL::FOpenGLFramebufferCache &Framebuffers;
    const Lumen::TVector<Lumen::TSharedPtr<Lumen::RHI::IRHIQueryPool>> &QueryPools;
    Lumen::RHI::FRHITimelineValue Completion;
    Lumen::RHIOpenGL::GLuint VertexArray           = 0U;
    Lumen::RHIOpenGL::GLuint InlineConstantsBuffer = 0U;
    Lumen::RHIOpenGL::GLuint CurrentFramebuffer    = 0U;
    Lumen::UInt32 FramebufferHeight                = 0U;
    Lumen::UInt64 IndexBufferOffset                = 0ULL;
    Lumen::RHIOpenGL::GLenum IndexType             = Lumen::RHIOpenGL::GlUnsignedInt;
    Lumen::UInt32 UniformUnitHighWater             = 0U;
    Lumen::UInt32 StorageUnitHighWater             = 0U;
    Lumen::UInt32 TextureUnitHighWater             = 0U;
    Lumen::UInt32 DebugGroupDepth                  = 0U;
    Lumen::Bool bRendering                         = false;
};

/** @brief Reads one typed payload and forwards it to a no-throw handler. */
template <typename PayloadType, typename HandlerType>
[[nodiscard]] Lumen::RHI::FRHIExpected WithPayload ( const Lumen::RHI::FRHICommandStream &InStream,
                                                     const Lumen::USize InIndex,
                                                     const Lumen::UInt32 InOpcode,
                                                     HandlerType &&InHandler ) noexcept
{
    LUMEN_EXPECT( Payload, InStream.ReadPayload<PayloadType>( InIndex, InOpcode ) );
    return InHandler( Payload );
}

[[nodiscard]] Lumen::UInt32 EffectiveColorCount ( const Lumen::RHIOpenGL::FOpenGLRenderingPayload &InDesc ) noexcept
{
    const Lumen::UInt32 Declared = InDesc.ColorAttachmentCount != 0U ? InDesc.ColorAttachmentCount : InDesc.Count;
    return Declared < Lumen::RHIOpenGL::MaxOpenGLCommandColorAttachments ? Declared
                                                                         : Lumen::RHIOpenGL::MaxOpenGLCommandColorAttachments;
}

[[nodiscard]] Lumen::Bool IsSrgbFormat ( const Lumen::RHIOpenGL::GLenum InFormat ) noexcept
{
    return InFormat == Lumen::RHIOpenGL::GlSrgb8Alpha8;
}

[[nodiscard]] Lumen::Int32
ToBottomLeft ( const Lumen::UInt32 InFramebufferHeight, const Lumen::Int32 InY, const Lumen::UInt32 InHeight ) noexcept
{
    const Lumen::Int64 Bottom = static_cast<Lumen::Int64>( InFramebufferHeight ) -
                                ( static_cast<Lumen::Int64>( InY ) + static_cast<Lumen::Int64>( InHeight ) );
    return static_cast<Lumen::Int32>( Bottom );
}

/**
 * @brief Accepts both the compact attachment record and the extended cache descriptor.
 */
[[nodiscard]] Lumen::TExpected<Lumen::RHIOpenGL::FOpenGLRenderingPayload, Lumen::RHI::FRHIError> DecodeRendering (
    const Lumen::RHI::FRHICommandStream &InStream, const Lumen::USize InIndex, const Lumen::UInt32 InOpcode ) noexcept
{
    const auto Full = InStream.ReadPayload<Lumen::RHIOpenGL::FOpenGLRenderingPayload>( InIndex, InOpcode );
    if ( Full )
    {
        return *Full;
    }

    LUMEN_EXPECT( Core, InStream.ReadPayload<Lumen::RHIOpenGL::FOpenGLRenderingCorePayload>( InIndex, InOpcode ) );

    Lumen::RHIOpenGL::FOpenGLRenderingPayload Result;
    Result.Area                 = Core.Area;
    Result.ColorAttachmentCount = Core.ColorAttachmentCount;
    Result.bHasDepthStencil     = Core.bHasDepthStencil;
    for ( Lumen::UInt32 Index = 0U; Index < Lumen::RHIOpenGL::MaxOpenGLCommandColorAttachments; ++Index )
    {
        Result.ColorViews[Index]   = Core.ColorViews[Index];
        Result.ColorFormats[Index] = Core.ColorFormats[Index];
    }
    Result.DepthView   = Core.DepthView;
    Result.DepthFormat = Core.DepthFormat;
    Result.Count       = Core.ColorAttachmentCount;
    return Result;
}

[[nodiscard]] Lumen::RHI::FRHIExpected ReplayBeginRendering ( const Lumen::RHI::FRHICommandStream &InStream,
                                                              const Lumen::USize InIndex,
                                                              const Lumen::UInt32 InOpcode,
                                                              FReplayState &InState ) noexcept
{
    LUMEN_EXPECT( Decoded, DecodeRendering( InStream, InIndex, InOpcode ) );
    const Lumen::RHIOpenGL::FOpenGLRenderingPayload &Payload = Decoded;

    LUMEN_EXPECT( Fbo, InState.Framebuffers.GetOrCreate( Payload ) );

    const Lumen::RHIOpenGL::GLuint Framebuffer = Fbo;
    InState.CurrentFramebuffer                 = Framebuffer;
    InState.FramebufferHeight                  = Payload.Area.Size.Height;
    InState.bRendering                         = true;

    InState.Dispatch.BindFramebuffer( Lumen::RHIOpenGL::GlDrawFramebuffer, Framebuffer );

    const Lumen::UInt32 ColorCount = EffectiveColorCount( Payload );
    const Lumen::Bool bSrgb =
        Lumen::Algo::AnyOf( Lumen::TSpan<const Lumen::RHIOpenGL::GLenum>( Payload.ColorFormats, ColorCount ),
                            [] ( const Lumen::RHIOpenGL::GLenum InFormat ) noexcept -> Lumen::Bool
    {
        return IsSrgbFormat( InFormat );
    } );
    InState.State.SetEnabled( Lumen::RHIOpenGL::GlFramebufferSrgb, bSrgb );
    InState.State.SetEnabled( Lumen::RHIOpenGL::GlMultisample, true );

    const Lumen::Int32 AreaX = Payload.Area.Position.X;
    const Lumen::Int32 AreaY = ToBottomLeft( Payload.Area.Size.Height, Payload.Area.Position.Y, Payload.Area.Size.Height );
    const Lumen::RHIOpenGL::GLsizei AreaWidth  = static_cast<Lumen::RHIOpenGL::GLsizei>( Payload.Area.Size.Width );
    const Lumen::RHIOpenGL::GLsizei AreaHeight = static_cast<Lumen::RHIOpenGL::GLsizei>( Payload.Area.Size.Height );

    InState.State.SetViewport( AreaX, AreaY, AreaWidth, AreaHeight );
    InState.State.SetScissor( false, AreaX, AreaY, AreaWidth, AreaHeight );
    if ( Payload.bClearColor or Payload.bClearDepthStencil )
    {
        InState.State.SetScissor( true, AreaX, AreaY, AreaWidth, AreaHeight );
        if ( Payload.bClearColor )
        {
            for ( Lumen::UInt32 Index = 0U; Index < ColorCount; ++Index )
            {
                InState.Dispatch.ClearNamedFramebufferfv( Framebuffer, GlColorBuffer,
                                                          static_cast<Lumen::RHIOpenGL::GLint>( Index ), Payload.ClearColor );
            }
        }
        if ( Payload.bClearDepthStencil )
        {
            InState.Dispatch.ClearNamedFramebufferfi( Framebuffer, GlDepthStencilBuffer, 0, Payload.ClearDepth,
                                                      Payload.ClearStencil );
        }
        InState.State.SetScissor( false, AreaX, AreaY, AreaWidth, AreaHeight );
    }
    return {};
}

[[nodiscard]] Lumen::RHI::FRHIExpected ReplayEndRendering ( const Lumen::RHI::FRHICommandStream &InStream,
                                                            const Lumen::USize InIndex,
                                                            const Lumen::UInt32 InOpcode,
                                                            FReplayState &InState ) noexcept
{
    return InStream.Read( InIndex ).AndThen(
        [InOpcode, &InState] ( const Lumen::RHI::FRHICommandView &InView ) noexcept -> Lumen::RHI::FRHIExpected
    {
        if ( InView.Opcode != InOpcode )
        {
            return Lumen::MakeUnexpected(
                Lumen::RHI::MakeRHICorrupted( Lumen::RHI::ERHIErrorCondition::Command, "EndRendering opcode mismatch." ) );
        }

        Lumen::RHIOpenGL::FOpenGLEndRenderingPayload Payload;
        if ( InView.Payload.size() >= sizeof( Payload ) )
        {
            std::memcpy( &Payload, InView.Payload.data(), sizeof( Payload ) );
        }

        const Lumen::RHIOpenGL::GLuint Framebuffer = Payload.Framebuffer != 0U ? Payload.Framebuffer : InState.CurrentFramebuffer;
        if ( Framebuffer != 0U )
        {
            InState.Framebuffers.NotifyEndRendering( Framebuffer, Payload.bInvalidate );
        }
        InState.State.SetEnabled( Lumen::RHIOpenGL::GlFramebufferSrgb, false );
        InState.State.SetEnabled( Lumen::RHIOpenGL::GlMultisample, false );
        InState.CurrentFramebuffer = 0U;
        InState.FramebufferHeight  = 0U;
        InState.bRendering         = false;
        return {};
    } );
}

[[nodiscard]] Lumen::RHI::FRHIExpected ReplayCommand ( const Lumen::RHI::FRHICommandView &InView,
                                                       const Lumen::RHI::FRHICommandStream &InStream,
                                                       const Lumen::USize InIndex,
                                                       FReplayState &InState ) noexcept
{
    using Lumen::RHIOpenGL::EOpenGLCommandOpcode;

    const Lumen::UInt32 Opcode = InView.Opcode;
    switch ( static_cast<EOpenGLCommandOpcode>( Opcode ) )
    {
    case EOpenGLCommandOpcode::Transition:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLTransitionPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InPayload.BarrierBits != 0U )
            {
                InState.Dispatch.MemoryBarrier( InPayload.BarrierBits );
            }
            return {};
        } );

    case EOpenGLCommandOpcode::CopyBuffer:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLBufferCopyPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InPayload.Size == 0U or InPayload.Source == 0U or InPayload.Destination == 0U )
            {
                return {};
            }
            InState.Dispatch.CopyNamedBufferSubData( InPayload.Source, InPayload.Destination,
                                                     static_cast<Lumen::RHIOpenGL::GLintptr>( InPayload.SourceOffset ),
                                                     static_cast<Lumen::RHIOpenGL::GLintptr>( InPayload.DestinationOffset ),
                                                     static_cast<Lumen::RHIOpenGL::GLsizeiptr>( InPayload.Size ) );
            return {};
        } );

    case EOpenGLCommandOpcode::CopyTexture:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLCopyTexturePayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InPayload.Source == 0U or InPayload.Destination == 0U or InPayload.Width == 0U )
            {
                return {};
            }
            InState.Dispatch.CopyImageSubData(
                InPayload.Source, InPayload.SourceTarget, InPayload.SourceLevel, InPayload.SourceX, InPayload.SourceY,
                InPayload.SourceZ, InPayload.Destination, InPayload.DestinationTarget, InPayload.DestinationLevel,
                InPayload.DestinationX, InPayload.DestinationY, InPayload.DestinationZ,
                static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.Width ),
                static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.Height ),
                static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.Depth == 0U ? 1U : InPayload.Depth ) );
            return {};
        } );

    case EOpenGLCommandOpcode::CopyTextureToBuffer:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLBufferTextureCopyPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InPayload.Buffer == 0U or InPayload.Texture == 0U or InPayload.Width == 0U )
            {
                return {};
            }
            if ( InState.Dispatch.GetTextureImage == nullptr )
            {
                return Lumen::MakeUnexpected( Lumen::RHI::MakeRHIUnsupported( Lumen::RHI::ERHIErrorCondition::Command,
                                                                              "Texture readback entry point is unavailable." ) );
            }
            const Lumen::UInt32 BytesPerPixel     = InPayload.BytesPerPixel != 0U ? InPayload.BytesPerPixel : 4U;
            const Lumen::UInt32 RowPixels         = InPayload.RowLength != 0U ? InPayload.RowLength : InPayload.Width;
            const Lumen::UInt32 Rows              = InPayload.Height != 0U ? InPayload.Height : 1U;
            const Lumen::RHIOpenGL::GLsizei Bytes = static_cast<Lumen::RHIOpenGL::GLsizei>( RowPixels * Rows * BytesPerPixel );
            InState.State.BindBuffer( Lumen::RHIOpenGL::GlPixelPackBuffer, InPayload.Buffer );
            InState.Dispatch.PixelStorei( Lumen::RHIOpenGL::GlPackRowLength,
                                          static_cast<Lumen::RHIOpenGL::GLint>( InPayload.RowLength ) );
            InState.Dispatch.GetTextureImage( InPayload.Texture, InPayload.Level, InPayload.Format, InPayload.Type, Bytes,
                                              reinterpret_cast<void *>( static_cast<Lumen::UPtr>( InPayload.BufferOffset ) ) );
            InState.Dispatch.PixelStorei( Lumen::RHIOpenGL::GlPackRowLength, 0 );
            InState.State.BindBuffer( Lumen::RHIOpenGL::GlPixelPackBuffer, 0U );
            return {};
        } );

    case EOpenGLCommandOpcode::CopyBufferToTexture:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLBufferTextureCopyPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InPayload.Buffer == 0U or InPayload.Texture == 0U or InPayload.Width == 0U )
            {
                return {};
            }
            if ( InPayload.Depth > 1U )
            {
                return Lumen::MakeUnexpected( Lumen::RHI::MakeRHIUnsupported(
                    Lumen::RHI::ERHIErrorCondition::Command, "Three-dimensional buffer/texture transfers are unavailable." ) );
            }
            InState.State.BindBuffer( Lumen::RHIOpenGL::GlPixelUnpackBuffer, InPayload.Buffer );
            InState.Dispatch.PixelStorei( GlUnpackRowLength, static_cast<Lumen::RHIOpenGL::GLint>( InPayload.RowLength ) );
            InState.Dispatch.TextureSubImage2D(
                InPayload.Texture, InPayload.Level, InPayload.X, InPayload.Y,
                static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.Width ),
                static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.Height == 0U ? 1U : InPayload.Height ), InPayload.Format,
                InPayload.Type, reinterpret_cast<const void *>( static_cast<Lumen::UPtr>( InPayload.BufferOffset ) ) );
            InState.Dispatch.PixelStorei( GlUnpackRowLength, 0 );
            InState.State.BindBuffer( Lumen::RHIOpenGL::GlPixelUnpackBuffer, 0U );
            return {};
        } );

    case EOpenGLCommandOpcode::BeginRendering:
        return ReplayBeginRendering( InStream, InIndex, Opcode, InState );

    case EOpenGLCommandOpcode::EndRendering:
        return ReplayEndRendering( InStream, InIndex, Opcode, InState );

    case EOpenGLCommandOpcode::SetPipeline:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLSetPipelinePayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            InState.State.UseProgram( InPayload.Program );
            if ( not InPayload.bIsCompute and InPayload.VertexArray != 0U )
            {
                InState.State.BindVertexArray( InPayload.VertexArray );
                InState.VertexArray = InPayload.VertexArray;
            }
            return {};
        } );

    case EOpenGLCommandOpcode::SetBindingSet:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLBindingSetPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            const Lumen::UInt32 UniformEnd = InPayload.UniformBufferBase + InPayload.UniformBufferCount;
            const Lumen::UInt32 StorageEnd = InPayload.StorageBufferBase + InPayload.StorageBufferCount;
            const Lumen::UInt32 TextureEnd = InPayload.TextureBase + InPayload.TextureCount;
            for ( Lumen::UInt32 Index = UniformEnd; Index < InState.UniformUnitHighWater; ++Index )
            {
                InState.State.BindBufferBase( Lumen::RHIOpenGL::GlUniformBuffer, Index, 0U );
            }
            for ( Lumen::UInt32 Index = StorageEnd; Index < InState.StorageUnitHighWater; ++Index )
            {
                InState.State.BindBufferBase( Lumen::RHIOpenGL::GlShaderStorageBuffer, Index, 0U );
            }
            for ( Lumen::UInt32 Index = TextureEnd; Index < InState.TextureUnitHighWater; ++Index )
            {
                InState.State.BindTextureUnit( Index, 0U );
                InState.State.BindSampler( Index, 0U );
            }
            InState.UniformUnitHighWater = UniformEnd > InState.UniformUnitHighWater ? UniformEnd : InState.UniformUnitHighWater;
            InState.StorageUnitHighWater = StorageEnd > InState.StorageUnitHighWater ? StorageEnd : InState.StorageUnitHighWater;
            InState.TextureUnitHighWater = TextureEnd > InState.TextureUnitHighWater ? TextureEnd : InState.TextureUnitHighWater;

            for ( Lumen::UInt32 Index = 0U; Index < InPayload.UniformBufferCount; ++Index )
            {
                if ( InPayload.UniformBuffers[Index] != 0U )
                {
                    InState.State.BindBufferBase( Lumen::RHIOpenGL::GlUniformBuffer, InPayload.UniformBufferBase + Index,
                                                  InPayload.UniformBuffers[Index] );
                }
            }
            for ( Lumen::UInt32 Index = 0U; Index < InPayload.StorageBufferCount; ++Index )
            {
                if ( InPayload.ShaderStorageBuffers[Index] != 0U )
                {
                    InState.State.BindBufferBase( Lumen::RHIOpenGL::GlShaderStorageBuffer, InPayload.StorageBufferBase + Index,
                                                  InPayload.ShaderStorageBuffers[Index] );
                }
            }
            for ( Lumen::UInt32 Index = 0U; Index < InPayload.TextureCount; ++Index )
            {
                if ( InPayload.Textures[Index] != 0U )
                {
                    InState.State.BindTextureUnit( InPayload.TextureBase + Index, InPayload.Textures[Index] );
                }
                if ( InPayload.Samplers[Index] != 0U )
                {
                    InState.State.BindSampler( InPayload.TextureBase + Index, InPayload.Samplers[Index] );
                }
            }
            return {};
        } );

    case EOpenGLCommandOpcode::SetInlineConstants:
    {
        return WithPayload<Lumen::RHIOpenGL::FOpenGLInlineConstantPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InState.InlineConstantsBuffer != 0U and InPayload.Size != 0U and InState.Dispatch.NamedBufferSubData != nullptr )
            {
                InState.Dispatch.NamedBufferSubData(
                    InState.InlineConstantsBuffer, static_cast<Lumen::RHIOpenGL::GLintptr>( InPayload.Offset ),
                    static_cast<Lumen::RHIOpenGL::GLsizeiptr>( InPayload.Size ), InPayload.Values );
            }
            return {};
        } );
    }

    case EOpenGLCommandOpcode::SetVertexBuffer:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLVertexBufferPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InPayload.Buffer == 0U or InState.VertexArray == 0U )
            {
                return {};
            }
            InState.Dispatch.VertexArrayVertexBuffer( InState.VertexArray, InPayload.Slot, InPayload.Buffer,
                                                      static_cast<Lumen::RHIOpenGL::GLintptr>( InPayload.Offset ),
                                                      static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.Stride ) );
            return {};
        } );

    case EOpenGLCommandOpcode::SetIndexBuffer:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLIndexBufferPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InPayload.Buffer == 0U or InState.VertexArray == 0U )
            {
                return {};
            }
            InState.Dispatch.VertexArrayElementBuffer( InState.VertexArray, InPayload.Buffer );
            InState.IndexBufferOffset = InPayload.Offset;
            InState.IndexType         = InPayload.IndexType;
            return {};
        } );

    case EOpenGLCommandOpcode::Draw:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLDrawPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            InState.Dispatch.DrawArrays( Lumen::RHIOpenGL::GlTriangles,
                                         static_cast<Lumen::RHIOpenGL::GLint>( InPayload.FirstVertex ),
                                         static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.VertexCount ) );
            return {};
        } );

    case EOpenGLCommandOpcode::DrawIndexed:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLDrawIndexedPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            const Lumen::UInt32 ElementSize = InPayload.IndexType == Lumen::RHIOpenGL::GlUnsignedShort ? 2U : 4U;
            const Lumen::UInt64 ByteOffset =
                InState.IndexBufferOffset + ( static_cast<Lumen::UInt64>( InPayload.FirstIndex ) * ElementSize );
            InState.Dispatch.DrawElements( Lumen::RHIOpenGL::GlTriangles,
                                           static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.IndexCount ), InPayload.IndexType,
                                           reinterpret_cast<const void *>( static_cast<Lumen::UPtr>( ByteOffset ) ) );
            return {};
        } );

    case EOpenGLCommandOpcode::DrawIndirect:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLIndirectPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            const void *Indirect = reinterpret_cast<const void *>( static_cast<Lumen::UPtr>( InPayload.Offset ) );
            if ( InPayload.bIndexed )
            {
                InState.Dispatch.DrawElementsIndirect( Lumen::RHIOpenGL::GlTriangles, InPayload.IndexType, Indirect );
            }
            else
            {
                InState.Dispatch.DrawArraysIndirect( Lumen::RHIOpenGL::GlTriangles, Indirect );
            }
            return {};
        } );

    case EOpenGLCommandOpcode::Dispatch:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLDispatchPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            InState.Dispatch.DispatchCompute( InPayload.X, InPayload.Y, InPayload.Z );
            return {};
        } );

    case EOpenGLCommandOpcode::DispatchIndirect:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLIndirectPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            InState.Dispatch.DispatchComputeIndirect( static_cast<Lumen::RHIOpenGL::GLintptr>( InPayload.Offset ) );
            return {};
        } );

    case EOpenGLCommandOpcode::SetViewport:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLViewportPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            InState.State.SetViewport( static_cast<Lumen::Int32>( InPayload.X ),
                                       ToBottomLeft( InState.FramebufferHeight, static_cast<Lumen::Int32>( InPayload.Y ),
                                                     static_cast<Lumen::UInt32>( InPayload.Height ) ),
                                       static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.Width ),
                                       static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.Height ) );
            if ( InState.Dispatch.DepthRangef != nullptr )
            {
                InState.Dispatch.DepthRangef( InPayload.MinDepth, InPayload.MaxDepth );
            }
            return {};
        } );

    case EOpenGLCommandOpcode::SetScissor:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLScissorPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            InState.State.SetScissor( true, InPayload.X, ToBottomLeft( InState.FramebufferHeight, InPayload.Y, InPayload.Height ),
                                      static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.Width ),
                                      static_cast<Lumen::RHIOpenGL::GLsizei>( InPayload.Height ) );
            return {};
        } );

    case EOpenGLCommandOpcode::SetBlendConstants:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLFloat4Payload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InState.Dispatch.BlendColor != nullptr )
            {
                InState.Dispatch.BlendColor( InPayload.Values[0], InPayload.Values[1], InPayload.Values[2], InPayload.Values[3] );
            }
            return {};
        } );

    case EOpenGLCommandOpcode::SetDepthBias:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLDepthBiasPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InState.Dispatch.PolygonOffset != nullptr )
            {
                InState.Dispatch.PolygonOffset( InPayload.Slope, InPayload.Constant );
                InState.State.SetEnabled( Lumen::RHIOpenGL::GlPolygonOffsetFill, true );
            }
            return {};
        } );

    case EOpenGLCommandOpcode::SetStencilReference:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLUIntPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InState.Dispatch.StencilFunc != nullptr )
            {
                InState.Dispatch.StencilFunc( Lumen::RHIOpenGL::GlAlways, static_cast<Lumen::RHIOpenGL::GLint>( InPayload.Value ),
                                              0xFFU );
                InState.State.SetEnabled( Lumen::RHIOpenGL::GlStencilTest, true );
            }
            return {};
        } );

    case EOpenGLCommandOpcode::BeginQuery:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLQueryPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InPayload.Pool == nullptr or InPayload.Query == 0U )
            {
                return Lumen::MakeUnexpected( Lumen::RHI::MakeRHIInvalidQuery( "The query payload is incomplete." ) );
            }
            /** INFO: an absolute timestamp is captured by glQueryCounter at EndQuery; BeginQuery only marks CPU state. */
            if ( InPayload.Target == Lumen::RHIOpenGL::GlTimestamp )
            {
                return {};
            }
            InState.Dispatch.BeginQuery( InPayload.Target, InPayload.Query );
            return {};
        } );

    case EOpenGLCommandOpcode::EndQuery:
        return WithPayload<Lumen::RHIOpenGL::FOpenGLQueryPayload>(
            InStream, InIndex, Opcode, [&InState] ( const auto &InPayload ) noexcept -> Lumen::RHI::FRHIExpected
        {
            if ( InPayload.Pool == nullptr or InPayload.PoolIndex >= InState.QueryPools.size() )
            {
                return Lumen::MakeUnexpected( Lumen::RHI::MakeRHIInvalidQuery( "The query pool snapshot is missing." ) );
            }
            const Lumen::TSharedPtr<Lumen::RHI::IRHIQueryPool> RetainedPool = InState.QueryPools[InPayload.PoolIndex];
            if ( not RetainedPool )
            {
                return Lumen::MakeUnexpected( Lumen::RHI::MakeRHIInvalidQuery( "The query pool was released." ) );
            }
            if ( InPayload.Target == Lumen::RHIOpenGL::GlTimestamp )
            {
                InState.Dispatch.QueryCounter( InPayload.Query, InPayload.Target );
            }
            else
            {
                InState.Dispatch.EndQuery( InPayload.Target );
            }

            const Lumen::RHIOpenGL::FGLDispatchTable &Dispatch = InState.Dispatch;
            const Lumen::RHI::FRHITimelineValue Completion     = InState.Completion;
            auto Poll = [&Dispatch, RetainedPool, Query = InPayload.Query, Index = InPayload.Index,
                         Generation = InPayload.Generation, Completion,
                         Attempts   = Lumen::USize{ 0U }] () mutable noexcept -> Lumen::Bool
            {
                Lumen::RHIOpenGL::GLuint Available = 0U;
                Dispatch.GetQueryObjectuiv( Query, Lumen::RHIOpenGL::GlQueryResultAvailable, &Available );
                Lumen::RHIOpenGL::GLuint64 Value = 0ULL;
                if ( Available != 0U )
                {
                    Dispatch.GetQueryObjectui64v( Query, Lumen::RHIOpenGL::GlQueryResult, &Value );
                }
                else if ( ++Attempts < 10000U )
                {
                    return false;
                }
                /* Give up: publish a zero result so the slot leaves Pending and can be reused. */
                const Lumen::TOptional<Lumen::RHIOpenGL::FOpenGLQueryPool *> Native =
                    Lumen::RHI::TryUnwrapRHIBackend<Lumen::RHIOpenGL::FOpenGLQueryPool>( RetainedPool.Get() );
                if ( Native )
                {
                    ( void )( *Native )->PublishResult( Index, Generation, static_cast<Lumen::UInt64>( Value ), Completion );
                }
                return true;
            };
            InState.Context.GetExecutor()->TrackQueryPoll( Lumen::TFunction<Lumen::Bool()>( Poll ) );
            return {};
        } );

    case EOpenGLCommandOpcode::BeginDebugRegion:
    {
        LUMEN_EXPECT( View, InStream.Read( InIndex ) );
        if ( View.Payload.size() >= sizeof( Lumen::RHIOpenGL::FOpenGLDebugRegionPayload ) )
        {
            Lumen::RHIOpenGL::FOpenGLDebugRegionPayload Header{};
            std::memcpy( &Header, View.Payload.data(), sizeof( Header ) );
            const Lumen::USize NameLength = static_cast<Lumen::USize>( Header.Length );
            if ( View.Payload.size() >= sizeof( Header ) + NameLength )
            {
                const Lumen::FStringView Label(
                    reinterpret_cast<const Lumen::AnsiChar *>( View.Payload.data() + sizeof( Header ) ), NameLength );
                InState.Context.GetExecutor()->EmitDebugMessage( Lumen::RHI::FRHIDebugMessage(
                    Lumen::RHI::ERHIDebugSeverity::Info, Lumen::RHI::ERHIDebugCategory::General, 0U, Label ) );
                if ( InState.Dispatch.DebugMessagePushGroup != nullptr )
                {
                    Lumen::AnsiChar Name[128]     = {};
                    const Lumen::USize CopyLength = NameLength < 127U ? NameLength : 127U;
                    std::memcpy( Name, View.Payload.data() + sizeof( Header ), CopyLength );
                    InState.Dispatch.DebugMessagePushGroup( 0x8245U, 0U, static_cast<Lumen::RHIOpenGL::GLsizei>( CopyLength ),
                                                            Name );
                    ++InState.DebugGroupDepth;
                }
            }
        }
        return {};
    }

    case EOpenGLCommandOpcode::EndDebugRegion:
    {
        LUMEN_TRY_EXPECT_ERROR( InStream.Read( InIndex ) );
        if ( InState.DebugGroupDepth != 0U and InState.Dispatch.DebugMessagePopGroup != nullptr )
        {
            InState.Dispatch.DebugMessagePopGroup();
            --InState.DebugGroupDepth;
        }
        return {};
    }

    default:
        return Lumen::MakeUnexpected(
            Lumen::RHI::MakeRHIUnsupported( Lumen::RHI::ERHIErrorCondition::Command, "Unsupported OpenGL command opcode." ) );
    }
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLCommandExecutor>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLCommandExecutor::Create ( TSharedRef<FOpenGLDeviceContext> InContext ) noexcept
{
    TSharedRef<FOpenGLCommandExecutor> Executor =
        MakeShared<FOpenGLCommandExecutor>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) );

    const FGLDispatchTable &Dispatch     = Executor->Context->GetDispatch();
    FOpenGLStateCache *State             = Executor->StateCache.Get();
    Lumen::RHIOpenGL::GLuint Buffer      = 0U;
    Lumen::RHIOpenGL::GLuint VertexArray = 0U;
    const RHI::FRHIExpected Created =
        Executor->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, State, &Buffer, &VertexArray] () noexcept
    {
        if ( Dispatch.CreateBuffers != nullptr )
        {
            Dispatch.CreateBuffers( 1, &Buffer );
            if ( Buffer != 0U and Dispatch.NamedBufferStorage != nullptr )
            {
                Dispatch.NamedBufferStorage( Buffer, static_cast<GLsizeiptr>( 64LL * 1024LL ), nullptr, GlDynamicStorageBit );
                State->BindBufferBase( GlUniformBuffer, 0U, Buffer );
            }
        }
        if ( Dispatch.CreateVertexArrays != nullptr )
        {
            Dispatch.CreateVertexArrays( 1, &VertexArray );
            if ( VertexArray != 0U )
            {
                State->BindVertexArray( VertexArray );
            }
        }
    } );
    LUMEN_TRY_EXPECT_ERROR( Created );
    if ( Buffer == 0U or VertexArray == 0U )
    {
        if ( Buffer != 0U or VertexArray != 0U )
        {
            ( void )Executor->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, Buffer, VertexArray] () noexcept
            {
                if ( Buffer != 0U and Dispatch.DeleteBuffers != nullptr )
                {
                    Dispatch.DeleteBuffers( 1, &Buffer );
                }
                if ( VertexArray != 0U and Dispatch.DeleteVertexArrays != nullptr )
                {
                    Dispatch.DeleteVertexArrays( 1, &VertexArray );
                }
            } );
        }
        return TUnexpected(
            RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Memory, "OpenGL could not allocate replay state buffers." ) );
    }

    Executor->VertexArray           = VertexArray;
    Executor->InlineConstantsBuffer = Buffer;
    return Executor;
}

Lumen::RHIOpenGL::FOpenGLCommandExecutor::FOpenGLCommandExecutor ( TSharedRef<FOpenGLDeviceContext> InContext ) noexcept
    : Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ),
      StateCache( MakeShared<FOpenGLStateCache>( Context->GetDispatch() ) ),
      FramebufferCache( MakeShared<FOpenGLFramebufferCache>( Context ) )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLCommandExecutor::~FOpenGLCommandExecutor () noexcept
{
    if ( VertexArray == 0U and InlineConstantsBuffer == 0U )
    {
        return;
    }
    const RHI::FRHITimelineValue Required     = Context->GetExecutor()->GetCompletedTimeline();
    const TSharedRef<FOpenGLContext> Executor = Context->GetExecutor();
    const FGLDispatchTable *Dispatch          = &Context->GetDispatch();
    const GLuint NativeBuffer                 = InlineConstantsBuffer;
    const GLuint NativeVertexArray            = VertexArray;
    ( void )Context->GetExecutor()->DeferRelease( Required, [Executor, Dispatch, NativeBuffer, NativeVertexArray] () noexcept
    {
        ( void )Executor->ExecuteOnGLThread( [Dispatch, NativeBuffer, NativeVertexArray] () noexcept
        {
            if ( NativeBuffer != 0U and Dispatch->DeleteBuffers != nullptr )
            {
                Dispatch->DeleteBuffers( 1, &NativeBuffer );
            }
            if ( NativeVertexArray != 0U and Dispatch->DeleteVertexArrays != nullptr )
            {
                Dispatch->DeleteVertexArrays( 1, &NativeVertexArray );
            }
        } );
    } );
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLCommandExecutor::Execute ( const RHI::FRHICommandStream &InStream,
                                                    const TVector<TSharedPtr<RHI::IRHIQueryPool>> &InQueryPools,
                                                    RHI::FRHITimelineValue InCompletion ) noexcept
{
    FReplayState Replay{ .Dispatch              = Context->GetDispatch(),
                         .Context               = *Context,
                         .State                 = *StateCache,
                         .Framebuffers          = *FramebufferCache,
                         .QueryPools            = InQueryPools,
                         .Completion            = InCompletion,
                         .VertexArray           = VertexArray,
                         .InlineConstantsBuffer = InlineConstantsBuffer };
    RHI::FRHIExpected ReplayResult;
    const RHI::FRHIExpected Posted = Context->GetExecutor()->ExecuteOnGLThread( [&InStream, &Replay, &ReplayResult] () noexcept
    {
        const USize CommandCount = InStream.GetCommandCount();
        for ( USize Index = 0U; Index < CommandCount; ++Index )
        {
            const auto View = InStream.Read( Index );
            if ( not View )
            {
                ReplayResult = Lumen::MakeUnexpected( View.Error() );
                break;
            }
            const RHI::FRHIExpected Replayed = ReplayCommand( *View, InStream, Index, Replay );
            if ( not Replayed )
            {
                ReplayResult = Lumen::MakeUnexpected( Replayed.Error() );
                break;
            }
            const Lumen::RHIOpenGL::GLenum NativeError = Replay.Dispatch.GetError();
            if ( NativeError != Lumen::RHIOpenGL::GlNoError )
            {
                ReplayResult =
                    Lumen::MakeUnexpected( RHI::FRHIError( RHI::ERHIErrorCode::BackendFailure, RHI::ERHIErrorCondition::Command,
                                                           "OpenGL reported an error while replaying a command." ) );
                break;
            }
        }
    } );
    LUMEN_TRY_EXPECT_ERROR( Posted );
    LUMEN_TRY_EXPECT_ERROR( ReplayResult );
    return {};
}
