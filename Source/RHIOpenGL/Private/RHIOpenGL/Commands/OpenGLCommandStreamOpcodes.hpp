/**
 * @file OpenGLCommandStreamOpcodes.hpp
 * @brief Declares stable stream opcodes and trivially copyable command payloads.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <RHI/Types/RHIExtent.hpp>

#include <RHIOpenGL/Loader/GLSpec.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @enum EOpenGLCommandOpcode
     * @brief Stable opcode identifiers written into FRHICommandStream records.
     */
    enum class EOpenGLCommandOpcode : UInt32
    {
        Transition          = 1U,
        CopyBuffer          = 2U,
        CopyTexture         = 3U,
        CopyBufferToTexture = 4U,
        CopyTextureToBuffer = 5U,
        BeginRendering      = 6U,
        EndRendering        = 7U,
        SetPipeline         = 8U,
        SetBindingSet       = 9U,
        SetInlineConstants  = 10U,
        SetVertexBuffer     = 11U,
        SetIndexBuffer      = 12U,
        Draw                = 13U,
        DrawIndexed         = 14U,
        DrawIndirect        = 15U,
        Dispatch            = 16U,
        DispatchIndirect    = 17U,
        SetViewport         = 18U,
        SetScissor          = 19U,
        SetBlendConstants   = 20U,
        SetDepthBias        = 21U,
        SetStencilReference = 22U,
        BeginQuery          = 23U,
        EndQuery            = 24U,
        BeginDebugRegion    = 25U,
        EndDebugRegion      = 26U,
    };

    /** @brief Maximum number of color attachments carried in one payload. */
    inline constexpr UInt32 MaxOpenGLCommandColorAttachments = 8U;

    /** @brief Maximum number of flat units per resource class in a binding set payload. */
    inline constexpr UInt32 MaxOpenGLCommandBindingUnits = 8U;

    /** @brief Maximum number of bytes carried by one inline constant payload. */
    inline constexpr UInt32 MaxOpenGLInlineConstantBytes = 256U;

    /** @brief Global memory barrier mask for a Transition command. */
    struct FOpenGLTransitionPayload
    {
        GLbitfield BarrierBits = 0U;
    };

    /** @brief Buffer-to-buffer copy of already resolved native buffer names. */
    struct FOpenGLBufferCopyPayload
    {
        GLuint Source            = 0U;
        GLuint Destination       = 0U;
        UInt64 SourceOffset      = 0ULL;
        UInt64 DestinationOffset = 0ULL;
        UInt64 Size              = 0ULL;
    };

    /**
     * @struct FOpenGLRenderingCorePayload
     * @brief Common attachment layout shared by every rendering scope record.
     */
    struct FOpenGLRenderingCorePayload
    {
        RHI::FRHIRect2D Area;
        UInt32 ColorAttachmentCount = 0U;
        Bool bHasDepthStencil       = false;
        GLuint ColorViews[8]        = {};
        GLenum ColorFormats[8]      = {};
        GLuint DepthView            = 0U;
        GLenum DepthFormat          = GlNone;
    };

    /**
     * @struct FOpenGLRenderingPayload
     * @brief Fixed-size rendering scope descriptor consumed by the cache.
     * @details The leading fields mirror FOpenGLRenderingCorePayload exactly;
     *          Count is authoritative when nonzero and defaults to
     *          ColorAttachmentCount otherwise.
     */
    struct FOpenGLRenderingPayload
    {
        RHI::FRHIRect2D Area;
        UInt32 ColorAttachmentCount = 0U;
        Bool bHasDepthStencil       = false;
        GLuint ColorViews[8]        = {};
        GLenum ColorFormats[8]      = {};
        GLuint DepthView            = 0U;
        GLenum DepthFormat          = GlNone;
        UInt32 Count                = 0U;
        Float32 ClearColor[4]       = {};
        Float32 ClearDepth          = 1.0F;
        Int32 ClearStencil          = 0;
        Bool bClearColor            = false;
        Bool bClearDepthStencil     = false;
    };

    /** @brief Ends a rendering scope and selects attachment invalidation. */
    struct FOpenGLEndRenderingPayload
    {
        GLuint Framebuffer   = 0U;
        GLuint ColorViews[8] = {};
        UInt32 Count         = 0U;
        GLuint DepthView     = 0U;
        GLenum DepthFormat   = GlNone;
        Bool bInvalidate     = false;
    };

    /** @brief Image-to-image copy issued through glCopyImageSubData. */
    struct FOpenGLCopyTexturePayload
    {
        GLuint Source            = 0U;
        GLuint Destination       = 0U;
        GLenum SourceTarget      = GlTexture2D;
        GLenum DestinationTarget = GlTexture2D;
        Int32 SourceLevel        = 0;
        Int32 DestinationLevel   = 0;
        Int32 SourceX            = 0;
        Int32 SourceY            = 0;
        Int32 SourceZ            = 0;
        Int32 DestinationX       = 0;
        Int32 DestinationY       = 0;
        Int32 DestinationZ       = 0;
        UInt32 Width             = 0U;
        UInt32 Height            = 0U;
        UInt32 Depth             = 0U;
    };

    /** @brief Linear buffer-to-texture or texture-to-buffer transfer descriptor. */
    struct FOpenGLBufferTextureCopyPayload
    {
        GLuint Buffer        = 0U;
        UInt64 BufferOffset  = 0ULL;
        GLuint Texture       = 0U;
        GLenum TextureTarget = GlTexture2D;
        Int32 Level          = 0;
        Int32 X              = 0;
        Int32 Y              = 0;
        Int32 Z              = 0;
        UInt32 Width         = 0U;
        UInt32 Height        = 0U;
        UInt32 Depth         = 0U;
        UInt32 RowLength     = 0U;
        GLenum Format        = GlRgba;
        GLenum Type          = GlUnsignedByte;
        Bool bFromBuffer     = true;
    };

    /** @brief Program switch, with the compute flag retained for validation. */
    struct FOpenGLSetPipelinePayload
    {
        GLuint Program  = 0U;
        Bool bIsCompute = false;
    };

    /** @brief Flat unit assignments for one binding set. */
    struct FOpenGLBindingSetPayload
    {
        UInt32 UnitCount               = 0U;
        GLuint UniformBuffers[8]       = {};
        GLuint ShaderStorageBuffers[8] = {};
        GLuint Textures[8]             = {};
        GLuint Samplers[8]             = {};
    };

    /** @brief Fixed-size inline constant record; stream may carry variable bytes. */
    struct FOpenGLInlineConstantPayload
    {
        UInt32 Offset    = 0U;
        UInt32 Size      = 0U;
        Byte Values[256] = {};
    };

    /** @brief A native buffer handle plus its resolved sub-allocation offset. */
    struct FOpenGLBufferBindingPayload
    {
        GLuint Buffer = 0U;
        UInt64 Offset = 0ULL;
    };

    /** @brief Vertex buffer binding for one VAO binding index. */
    struct FOpenGLVertexBufferPayload
    {
        GLuint Buffer = 0U;
        UInt64 Offset = 0ULL;
        UInt32 Slot   = 0U;
        UInt32 Stride = 0U;
    };

    /** @brief Element array buffer binding and element type. */
    struct FOpenGLIndexBufferPayload
    {
        GLuint Buffer    = 0U;
        UInt64 Offset    = 0ULL;
        GLenum IndexType = GlUnsignedInt;
    };

    /** @brief Non-indexed draw arguments. */
    struct FOpenGLDrawPayload
    {
        UInt32 VertexCount   = 0U;
        UInt32 InstanceCount = 0U;
        UInt32 FirstVertex   = 0U;
        UInt32 FirstInstance = 0U;
    };

    /** @brief Indexed draw arguments. */
    struct FOpenGLDrawIndexedPayload
    {
        UInt32 IndexCount    = 0U;
        UInt32 InstanceCount = 0U;
        UInt32 FirstIndex    = 0U;
        Int32 VertexOffset   = 0;
        UInt32 FirstInstance = 0U;
        GLenum IndexType     = GlUnsignedInt;
    };

    /** @brief Indirect draw or dispatch arguments stored in a buffer. */
    struct FOpenGLIndirectPayload
    {
        GLuint Buffer    = 0U;
        UInt64 Offset    = 0ULL;
        UInt32 Count     = 0U;
        UInt32 Stride    = 0U;
        Bool bIndexed    = false;
        GLenum IndexType = GlUnsignedInt;
    };

    /** @brief Compute dispatch group counts. */
    struct FOpenGLDispatchPayload
    {
        UInt32 X = 0U;
        UInt32 Y = 0U;
        UInt32 Z = 0U;
    };

    /** @brief Viewport in RHI top-left coordinates and depth range. */
    struct FOpenGLViewportPayload
    {
        Float32 X        = 0.0F;
        Float32 Y        = 0.0F;
        Float32 Width    = 0.0F;
        Float32 Height   = 0.0F;
        Float32 MinDepth = 0.0F;
        Float32 MaxDepth = 1.0F;
    };

    /** @brief Scissor rectangle in RHI top-left coordinates. */
    struct FOpenGLScissorPayload
    {
        Int32 X       = 0;
        Int32 Y       = 0;
        UInt32 Width  = 0U;
        UInt32 Height = 0U;
    };

    /** @brief Four floating-point dynamic state values. */
    struct FOpenGLFloat4Payload
    {
        Float32 Values[4] = {};
    };

    /** @brief Polygon offset dynamic state. */
    struct FOpenGLDepthBiasPayload
    {
        Float32 Constant = 0.0F;
        Float32 Slope    = 0.0F;
    };

    /** @brief Generic unsigned dynamic state value. */
    struct FOpenGLUIntPayload
    {
        UInt32 Value = 0U;
    };

    /** @brief Native query handle plus its pool index. */
    struct FOpenGLQueryPayload
    {
        GLuint Query = 0U;
        UInt32 Index = 0U;
    };

    /** @brief Debug region name length; the name bytes follow the payload. */
    struct FOpenGLDebugRegionPayload
    {
        UInt32 Length = 0U;
    };

    /** @brief Payload for commands that carry no arguments. */
    struct FOpenGLEmptyPayload
    {
    };

} // namespace RHIOpenGL

} // namespace Lumen
