/**
 * @file GLSpec.hpp
 * @brief Declares the minimal Khronos OpenGL 4.6 / EGL ABI used by RHIOpenGL.
 */

#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /** @brief OpenGL scalars compatible with the Khronos GL ABI. */
    using GLenum     = UInt32;
    using GLuint     = UInt32;
    using GLuint64   = UInt64;
    using GLint      = Int32;
    using GLint64    = Int64;
    using GLubyte    = Byte;
    using GLsizei    = Int32;
    using GLintptr   = Int64;
    using GLsizeiptr = Int64;
    using GLbitfield = UInt32;
    using GLfloat    = Float32;
    using GLboolean  = UInt8;

    /** @brief Opaque sync object handle (core 4.6). */
    using GLsync = void *;

    /** @brief EGL opaque handles, represented as pointer-sized integers. */
    using EGLNativeDisplayType = UPtr;
    using EGLDisplay           = UPtr;
    using EGLConfig            = UPtr;
    using EGLContext           = UPtr;
    using EGLSurface           = UPtr;
    using EGLBoolean           = UInt32;
    using EGLint               = Int32;
    using EGLenum              = UInt32;

    inline constexpr EGLBoolean EglFalse = 0U;
    inline constexpr EGLBoolean EglTrue  = 1U;
    inline constexpr EGLint EglNone      = 0x3038;
    inline constexpr EGLint EglSuccess   = 0x3000;

    inline constexpr EGLenum EglOpenGLApi               = 0x30A2U;
    inline constexpr EGLenum EglPlatformDeviceExt       = 0x313FU;
    inline constexpr EGLenum EglPlatformSurfacelessMesa = 0x31DDU;

    inline constexpr EGLint EglContextMajorVersion         = 0x3098;
    inline constexpr EGLint EglContextMinorVersion         = 0x30FB;
    inline constexpr EGLint EglContextOpenGLProfileMask    = 0x9126;
    inline constexpr EGLint EglContextOpenGLCoreProfileBit = 0x0001;
    inline constexpr EGLint EglContextOpenGLDebugBit       = 0x0002;
    inline constexpr EGLint EglSurfaceType                 = 0x3033;
    inline constexpr EGLint EglPbufferBit                  = 0x0001;
    inline constexpr EGLint EglWindowBit                   = 0x0004;
    inline constexpr EGLint EglRenderableType              = 0x3040;
    inline constexpr EGLint EglOpenGLBit                   = 0x0008;
    inline constexpr EGLint EglRedSize                     = 0x3024;
    inline constexpr EGLint EglGreenSize                   = 0x3023;
    inline constexpr EGLint EglBlueSize                    = 0x3022;
    inline constexpr EGLint EglAlphaSize                   = 0x3028;
    inline constexpr EGLint EglDepthSize                   = 0x3025;
    inline constexpr EGLint EglWidth                       = 0x3057;
    inline constexpr EGLint EglHeight                      = 0x3056;
    inline constexpr EGLint EglExtensions                  = 0x3055;

    inline constexpr GLenum GlVendor                 = 0x1F00U;
    inline constexpr GLenum GlRenderer               = 0x1F01U;
    inline constexpr GLenum GlVersion                = 0x1F02U;
    inline constexpr GLenum GlExtensions             = 0x1F03U;
    inline constexpr GLenum GlShadingLanguageVersion = 0x8B8CU;
    inline constexpr GLenum GlNumExtensions          = 0x821DU;
    inline constexpr GLenum GlNoError                = 0U;
    inline constexpr GLenum GlContextLost            = 0x9242U;

    inline constexpr GLenum GlMaxTextureSize                        = 0x0D33U;
    inline constexpr GLenum GlMax3DTextureSize                      = 0x8073U;
    inline constexpr GLenum GlMaxArrayTextureLayers                 = 0x88FFU;
    inline constexpr GLenum GlMaxColorAttachments                   = 0x8CDFU;
    inline constexpr GLenum GlMaxVertexAttribs                      = 0x8869U;
    inline constexpr GLenum GlMaxVertexAttribBindings               = 0x82DAU;
    inline constexpr GLenum GlMaxComputeWorkGroupInvocations        = 0x90EBU;
    inline constexpr GLenum GlMaxUniformBufferOffsetAlignment       = 0x8A34U;
    inline constexpr GLenum GlMaxShaderStorageBufferOffsetAlignment = 0x90DFU;
    inline constexpr GLenum GlMaxShaderStorageBlockSize             = 0x90DEU;
    inline constexpr GLenum GlMaxCombinedTextureImageUnits          = 0x8B4DU;
    inline constexpr GLenum GlMaxCombinedUniformBlocks              = 0x8A2EU;
    inline constexpr GLenum GlMaxUniformBufferBindings              = 0x8A2FU;
    inline constexpr GLenum GlMaxShaderStorageBufferBindings        = 0x90DDU;
    inline constexpr GLenum GlMaxImageUnits                         = 0x8F38U;
    inline constexpr GLenum GlMaxTextureMaxAnisotropy               = 0x84FFU;
    inline constexpr GLenum GlMaxSamples                            = 0x8D57U;
    inline constexpr GLenum GlLowerLeft                             = 0x8CA1U;
    inline constexpr GLenum GlZeroToOne                             = 0x935FU;
    inline constexpr GLenum GlDepthClamp                            = 0x864FU;
    inline constexpr GLenum GlTextureCubeMapSeamless                = 0x884FU;

    /** INFO: Buffer storage flags (core 4.6). */
    inline constexpr GLbitfield GlMapReadBit        = 0x0001U;
    inline constexpr GLbitfield GlMapWriteBit       = 0x0002U;
    inline constexpr GLbitfield GlMapPersistentBit  = 0x0040U;
    inline constexpr GLbitfield GlMapCoherentBit    = 0x0080U;
    inline constexpr GLbitfield GlDynamicStorageBit = 0x0100U;
    inline constexpr GLbitfield GlClientStorageBit  = 0x0200U;

    /** INFO: Texture targets and formats needed by immutable storage and views. */
    inline constexpr GLenum GlTexture1D                 = 0x0DE0U;
    inline constexpr GLenum GlTexture2D                 = 0x0DE1U;
    inline constexpr GLenum GlTexture3D                 = 0x806FU;
    inline constexpr GLenum GlTextureCubeMap            = 0x8513U;
    inline constexpr GLenum GlTexture2DArray            = 0x8C1AU;
    inline constexpr GLenum GlTexture2DMultisample      = 0x9100U;
    inline constexpr GLenum GlTexture2DMultisampleArray = 0x9102U;
    inline constexpr GLenum GlRgba8                     = 0x8058U;
    inline constexpr GLenum GlSrgb8Alpha8               = 0x8C43U;
    inline constexpr GLenum GlRgba                      = 0x1908U;
    inline constexpr GLenum GlUnsignedByte              = 0x1401U;
    inline constexpr GLenum GlTextureViewMinLevel       = 0x82DBU;
    inline constexpr GLenum GlTextureViewNumLevels      = 0x82DCU;
    inline constexpr GLenum GlTextureViewMinLayer       = 0x82DDU;
    inline constexpr GLenum GlTextureViewNumLayers      = 0x82DEU;

    /** INFO: Sampler state (core 4.6 plus the ARB anisotropy constant). */
    inline constexpr GLenum GlTextureMinFilter     = 0x2801U;
    inline constexpr GLenum GlTextureMagFilter     = 0x2800U;
    inline constexpr GLenum GlTextureWrapS         = 0x2802U;
    inline constexpr GLenum GlTextureWrapT         = 0x2803U;
    inline constexpr GLenum GlTextureWrapR         = 0x8072U;
    inline constexpr GLenum GlTextureCompareMode   = 0x884CU;
    inline constexpr GLenum GlTextureCompareFunc   = 0x884DU;
    inline constexpr GLenum GlCompareRefToTexture  = 0x884EU;
    inline constexpr GLenum GlTextureMaxAnisotropy = 0x84FEU;
    inline constexpr GLenum GlNearest              = 0x2600U;
    inline constexpr GLenum GlLinear               = 0x2601U;
    inline constexpr GLenum GlRepeat               = 0x2901U;
    inline constexpr GLenum GlMirroredRepeat       = 0x8370U;
    inline constexpr GLenum GlClampToEdge          = 0x812FU;
    inline constexpr GLenum GlClampToBorder        = 0x812DU;
    inline constexpr GLenum GlNever                = 0x0200U;
    inline constexpr GLenum GlLess                 = 0x0201U;
    inline constexpr GLenum GlEqual                = 0x0202U;
    inline constexpr GLenum GlLessEqual            = 0x0203U;
    inline constexpr GLenum GlGreater              = 0x0204U;
    inline constexpr GLenum GlNotEqual             = 0x0205U;
    inline constexpr GLenum GlGreaterEqual         = 0x0206U;
    inline constexpr GLenum GlAlways               = 0x0207U;

    /* Shader and program objects (core 4.6). */
    inline constexpr GLenum GlVertexShader        = 0x8B31U;
    inline constexpr GLenum GlFragmentShader      = 0x8B30U;
    inline constexpr GLenum GlComputeShader       = 0x91B9U;
    inline constexpr GLenum GlCompileStatus       = 0x8B81U;
    inline constexpr GLenum GlLinkStatus          = 0x8B82U;
    inline constexpr GLenum GlInfoLogLength       = 0x8B84U;
    inline constexpr GLenum GlShaderStorageBlock  = 0x92E6U;
    inline constexpr GLenum GlUniformBlockBinding = 0x8A3FU;

    /* Buffer binding targets and global barriers (core 4.6). */
    inline constexpr GLenum GlArrayBuffer                      = 0x8892U;
    inline constexpr GLenum GlElementArrayBuffer               = 0x8893U;
    inline constexpr GLenum GlUniformBuffer                    = 0x8A11U;
    inline constexpr GLenum GlShaderStorageBuffer              = 0x90D2U;
    inline constexpr GLenum GlPixelUnpackBuffer                = 0x88ECU;
    inline constexpr GLenum GlPixelPackBuffer                  = 0x88EDU;
    inline constexpr GLbitfield GlVertexAttribArrayBarrierBit  = 0x00000001U;
    inline constexpr GLbitfield GlElementArrayBarrierBit       = 0x00000002U;
    inline constexpr GLbitfield GlUniformBarrierBit            = 0x00000004U;
    inline constexpr GLbitfield GlTextureFetchBarrierBit       = 0x00000008U;
    inline constexpr GLbitfield GlShaderImageAccessBarrierBit  = 0x00000020U;
    inline constexpr GLbitfield GlCommandBarrierBit            = 0x00000040U;
    inline constexpr GLbitfield GlBufferUpdateBarrierBit       = 0x00000200U;
    inline constexpr GLbitfield GlFramebufferBarrierBit        = 0x00000400U;
    inline constexpr GLbitfield GlShaderStorageBarrierBit      = 0x00002000U;
    inline constexpr GLbitfield GlClientMappedBufferBarrierBit = 0x00004000U;

    /* Framebuffer, clearing, drawing and fixed-function state (core 4.6). */
    inline constexpr GLenum GlDrawFramebuffer        = 0x8CA9U;
    inline constexpr GLenum GlReadFramebuffer        = 0x8CA8U;
    inline constexpr GLenum GlFramebufferComplete    = 0x8CD5U;
    inline constexpr GLenum GlColorAttachment0       = 0x8CE0U;
    inline constexpr GLenum GlDepthAttachment        = 0x8D00U;
    inline constexpr GLenum GlStencilAttachment      = 0x8D20U;
    inline constexpr GLenum GlDepthStencilAttachment = 0x821AU;
    inline constexpr GLenum GlNone                   = 0U;
    inline constexpr GLbitfield GlColorBufferBit     = 0x00004000U;
    inline constexpr GLbitfield GlDepthBufferBit     = 0x00000100U;
    inline constexpr GLbitfield GlStencilBufferBit   = 0x00000400U;
    inline constexpr GLenum GlTriangles              = 0x0004U;
    inline constexpr GLenum GlUnsignedShort          = 0x1403U;
    inline constexpr GLenum GlUnsignedInt            = 0x1405U;
    inline constexpr GLenum GlInt                    = 0x1404U;
    inline constexpr GLenum GlFloat                  = 0x1406U;
    inline constexpr GLenum GlScissorTest            = 0x0C11U;
    inline constexpr GLenum GlFramebufferSrgb        = 0x8DB9U;
    inline constexpr GLenum GlMultisample            = 0x809DU;

    /* Query objects (core 4.6). */
    inline constexpr GLenum GlSamplesPassed        = 0x8914U;
    inline constexpr GLenum GlAnySamplesPassed     = 0x8C22U;
    inline constexpr GLenum GlTimeElapsed          = 0x88BFU;
    inline constexpr GLenum GlTimestamp            = 0x8E28U;
    inline constexpr GLenum GlQueryResult          = 0x8866U;
    inline constexpr GLenum GlQueryResultAvailable = 0x8867U;
    inline constexpr GLenum GlPackRowLength        = 0x0D02U;

    /** INFO: Sync objects and client wait results (core 4.6); values match glcorearb.h. */
    inline constexpr GLenum GlSyncGpuCommandsComplete  = 0x9117U;
    inline constexpr GLenum GlSyncConditionSatisfied   = 0x911CU;
    inline constexpr GLenum GlSyncAlreadySignaled      = 0x911AU;
    inline constexpr GLenum GlSyncTimeoutExpired       = 0x911BU;
    inline constexpr GLenum GlSyncWaitFailed           = 0x911DU;
    inline constexpr GLbitfield GlSyncFlushCommandsBit = 0x00000001U;

    /** INFO: KHR_debug message routing (core 4.6). */
    inline constexpr GLenum GlDebugOutput         = 0x92E0U;
    inline constexpr GLenum GlDebugSourceApi      = 0x8246U;
    inline constexpr GLenum GlDebugTypeErrorCode  = 0x824AU;
    inline constexpr GLenum GlDebugSeverityHigh   = 0x9146U;
    inline constexpr GLenum GlDebugSeverityMedium = 0x9147U;
    inline constexpr GLenum GlDebugSeverityLow    = 0x9148U;
    inline constexpr GLenum GlDontCare            = 0x110BU;

    /** INFO: Dynamic state entry point constants (core 4.6). */
    inline constexpr GLenum GlBlendColorValue   = 0x8005U;
    inline constexpr GLenum GlBlend             = 0x0BE2U;
    inline constexpr GLenum GlDepthTest         = 0x0B71U;
    inline constexpr GLenum GlStencilTest       = 0x0B90U;
    inline constexpr GLenum GlPolygonOffsetFill = 0x8037U;
    inline constexpr GLenum GlZero              = 0U;
    inline constexpr GLenum GlOne               = 0x0001U;
    inline constexpr GLenum GlSrcAlpha          = 0x0302U;
    inline constexpr GLenum GlOneMinusSrcAlpha  = 0x0303U;
    inline constexpr GLenum GlFuncAdd           = 0x8006U;
    inline constexpr GLenum GlKeep              = 0x1E00U;
    inline constexpr GLenum GlReplace           = 0x1E01U;

    using TGLGetString              = const GLubyte *( * )( GLenum );
    using TGLGetStringi             = const GLubyte *( * )( GLenum, GLuint );
    using TGLGetIntegerv            = void ( * )( GLenum, GLint * );
    using TGLGetInteger64v          = void ( * )( GLenum, GLint64 * );
    using TGLGetError               = GLenum ( * )();
    using TGLClipControl            = void ( * )( GLenum, GLenum );
    using TGLGetGraphicsResetStatus = GLenum ( * )();
    using TGLFenceSync              = GLsync ( * )( GLenum, GLbitfield );
    using TGLDeleteSync             = void ( * )( GLsync );
    using TGLClientWaitSync         = GLenum ( * )( GLsync, GLbitfield, GLuint64 );
    using TGLFlush                  = void ( * )();
    using TGLDebugProc              = void ( * )( GLenum, GLenum, GLuint, GLenum, GLsizei, const AnsiChar *, const void * );
    using TGLDebugMessageCallback   = void ( * )( TGLDebugProc, const void * );
    using TGLDebugMessageControl    = void ( * )( GLenum, GLenum, GLenum, GLsizei, const GLuint *, GLboolean );
    using TGLPushDebugGroup         = void ( * )( GLenum, GLuint, GLsizei, const AnsiChar * );
    using TGLPopDebugGroup          = void ( * )();

    using TGLCreateBuffers               = void ( * )( GLsizei, GLuint * );
    using TGLDeleteBuffers               = void ( * )( GLsizei, const GLuint * );
    using TGLNamedBufferStorage          = void ( * )( GLuint, GLsizeiptr, const void *, GLbitfield );
    using TGLNamedBufferSubData          = void ( * )( GLuint, GLintptr, GLsizeiptr, const void * );
    using TGLMapNamedBufferRange         = void *( * )( GLuint, GLintptr, GLsizeiptr, GLbitfield );
    using TGLUnmapNamedBuffer            = GLboolean ( * )( GLuint );
    using TGLCopyNamedBufferSubData      = void ( * )( GLuint, GLuint, GLintptr, GLintptr, GLsizeiptr );
    using TGLCreateTextures              = void ( * )( GLenum, GLsizei, GLuint * );
    using TGLDeleteTextures              = void ( * )( GLsizei, const GLuint * );
    using TGLTextureStorage2D            = void ( * )( GLuint, GLsizei, GLenum, GLsizei, GLsizei );
    using TGLTextureStorage3D            = void ( * )( GLuint, GLsizei, GLenum, GLsizei, GLsizei, GLsizei );
    using TGLTextureStorage2DMultisample = void ( * )( GLuint, GLsizei, GLenum, GLsizei, GLsizei, GLboolean );
    using TGLTextureView                 = void ( * )( GLuint, GLenum, GLuint, GLenum, GLuint, GLuint, GLuint, GLuint );
    using TGLCreateSamplers              = void ( * )( GLsizei, GLuint * );
    using TGLDeleteSamplers              = void ( * )( GLsizei, const GLuint * );
    using TGLSamplerParameteri           = void ( * )( GLuint, GLenum, GLint );
    using TGLSamplerParameterf           = void ( * )( GLuint, GLenum, GLfloat );

    /* Shader, program and resource binding entry points (core 4.6). */
    using TGLCreateShader              = GLuint ( * )( GLenum );
    using TGLDeleteShader              = void ( * )( GLuint );
    using TGLShaderSource              = void ( * )( GLuint, GLsizei, const AnsiChar *const *, const GLint * );
    using TGLCompileShader             = void ( * )( GLuint );
    using TGLGetShaderiv               = void ( * )( GLuint, GLenum, GLint * );
    using TGLGetShaderInfoLog          = void ( * )( GLuint, GLsizei, GLsizei *, AnsiChar * );
    using TGLCreateProgram             = GLuint ( * )();
    using TGLDeleteProgram             = void ( * )( GLuint );
    using TGLAttachShader              = void ( * )( GLuint, GLuint );
    using TGLLinkProgram               = void ( * )( GLuint );
    using TGLGetProgramiv              = void ( * )( GLuint, GLenum, GLint * );
    using TGLGetProgramInfoLog         = void ( * )( GLuint, GLsizei, GLsizei *, AnsiChar * );
    using TGLUseProgram                = void ( * )( GLuint );
    using TGLUniformBlockBinding       = void ( * )( GLuint, GLuint, GLuint );
    using TGLShaderStorageBlockBinding = void ( * )( GLuint, GLuint, GLuint );
    using TGLGetProgramResourceIndex   = GLuint ( * )( GLuint, GLenum, const AnsiChar * );
    using TGLProgramUniform1i          = void ( * )( GLuint, GLint, GLint );
    using TGLGetUniformLocation        = GLint ( * )( GLuint, const AnsiChar * );
    using TGLBindBufferBase            = void ( * )( GLenum, GLuint, GLuint );
    using TGLBindBufferRange           = void ( * )( GLenum, GLuint, GLuint, GLintptr, GLsizeiptr );
    using TGLBindTextureUnit           = void ( * )( GLuint, GLuint );
    using TGLBindSampler               = void ( * )( GLuint, GLuint );
    using TGLMemoryBarrier             = void ( * )( GLbitfield );

    /* Vertex array objects (core 4.6 DSA). */
    using TGLCreateVertexArrays        = void ( * )( GLsizei, GLuint * );
    using TGLDeleteVertexArrays        = void ( * )( GLsizei, const GLuint * );
    using TGLBindVertexArray           = void ( * )( GLuint );
    using TGLVertexArrayAttribFormat   = void ( * )( GLuint, GLuint, GLint, GLenum, GLboolean, GLuint );
    using TGLVertexArrayAttribBinding  = void ( * )( GLuint, GLuint, GLuint );
    using TGLEnableVertexArrayAttrib   = void ( * )( GLuint, GLuint );
    using TGLVertexArrayBindingDivisor = void ( * )( GLuint, GLuint, GLuint );
    using TGLVertexArrayVertexBuffer   = void ( * )( GLuint, GLuint, GLuint, GLintptr, GLsizei );
    using TGLVertexArrayElementBuffer  = void ( * )( GLuint, GLuint );

    /* Framebuffer objects and fixed-function state (core 4.6 DSA). */
    using TGLCreateFramebuffers             = void ( * )( GLsizei, GLuint * );
    using TGLDeleteFramebuffers             = void ( * )( GLsizei, const GLuint * );
    using TGLBindFramebuffer                = void ( * )( GLenum, GLuint );
    using TGLNamedFramebufferTexture        = void ( * )( GLuint, GLenum, GLuint, GLint );
    using TGLNamedFramebufferTextureLayer   = void ( * )( GLuint, GLenum, GLuint, GLint, GLint );
    using TGLNamedFramebufferDrawBuffers    = void ( * )( GLuint, GLsizei, const GLenum * );
    using TGLNamedFramebufferReadBuffer     = void ( * )( GLuint, GLenum );
    using TGLCheckNamedFramebufferStatus    = GLenum ( * )( GLuint, GLenum );
    using TGLClearNamedFramebufferfv        = void ( * )( GLuint, GLenum, GLint, const GLfloat * );
    using TGLClearNamedFramebufferiv        = void ( * )( GLuint, GLenum, GLint, const GLint * );
    using TGLClearNamedFramebufferuiv       = void ( * )( GLuint, GLenum, GLint, const GLuint * );
    using TGLClearNamedFramebufferfi        = void ( * )( GLuint, GLenum, GLint, GLfloat, GLint );
    using TGLInvalidateNamedFramebufferData = void ( * )( GLuint, GLsizei, const GLenum * );
    using TGLViewport                       = void ( * )( GLint, GLint, GLsizei, GLsizei );
    using TGLDepthRangef                    = void ( * )( GLfloat, GLfloat );
    using TGLScissor                        = void ( * )( GLint, GLint, GLsizei, GLsizei );
    using TGLEnable                         = void ( * )( GLenum );
    using TGLDisable                        = void ( * )( GLenum );
    using TGLPixelStorei                    = void ( * )( GLenum, GLint );

    /* Draw commands (core 4.6). */
    using TGLDrawArrays              = void ( * )( GLenum, GLint, GLsizei );
    using TGLDrawElements            = void ( * )( GLenum, GLsizei, GLenum, const void * );
    using TGLDrawArraysIndirect      = void ( * )( GLenum, const void * );
    using TGLDrawElementsIndirect    = void ( * )( GLenum, GLenum, const void * );
    using TGLDispatchCompute         = void ( * )( GLuint, GLuint, GLuint );
    using TGLDispatchComputeIndirect = void ( * )( GLintptr );

    /* Texture and query commands (core 4.6). */
    using TGLTextureSubImage2D = void ( * )( GLuint, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void * );
    using TGLCopyImageSubData  = void ( * )(
        GLuint, GLenum, GLint, GLint, GLint, GLint, GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei );
    using TGLCreateQueries       = void ( * )( GLenum, GLsizei, GLuint * );
    using TGLDeleteQueries       = void ( * )( GLsizei, const GLuint * );
    using TGLBeginQuery          = void ( * )( GLenum, GLuint );
    using TGLEndQuery            = void ( * )( GLenum );
    using TGLQueryCounter        = void ( * )( GLuint, GLenum );
    using TGLGetQueryObjectuiv   = void ( * )( GLuint, GLenum, GLuint * );
    using TGLGetQueryObjectui64v = void ( * )( GLuint, GLenum, GLuint64 * );
    using TGLGetQueryiv          = void ( * )( GLenum, GLenum, GLint * );
    using TGLGetTextureImage     = void ( * )( GLuint, GLint, GLenum, GLenum, GLsizei, void * );

    /* Dynamic state and readback entry points (core 4.6). */
    using TGLBlendColor            = void ( * )( GLfloat, GLfloat, GLfloat, GLfloat );
    using TGLBlendEquationSeparate = void ( * )( GLenum, GLenum );
    using TGLBlendFuncSeparate     = void ( * )( GLenum, GLenum, GLenum, GLenum );
    using TGLPolygonOffset         = void ( * )( GLfloat, GLfloat );
    using TGLStencilFunc           = void ( * )( GLenum, GLint, GLuint );
    using TGLGetBufferSubData      = void ( * )( GLenum, GLintptr, GLsizeiptr, void * );
    using TGLBindBuffer            = void ( * )( GLenum, GLuint );
    using TGLBlitFramebuffer       = void ( * )( GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum );

    using TEGLGetProcAddress        = void *( * )( const AnsiChar * );
    using TEGLGetDisplay            = EGLDisplay ( * )( EGLNativeDisplayType );
    using TEGLGetPlatformDisplayEXT = EGLDisplay ( * )( EGLenum, void *, const EGLint * );
    using TEGLInitialize            = EGLBoolean ( * )( EGLDisplay, EGLint *, EGLint * );
    using TEGLTerminate             = EGLBoolean ( * )( EGLDisplay );
    using TEGLBindAPI               = EGLBoolean ( * )( EGLenum );
    using TEGLChooseConfig          = EGLBoolean ( * )( EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint * );
    using TEGLCreateContext         = EGLContext ( * )( EGLDisplay, EGLConfig, EGLContext, const EGLint * );
    using TEGLDestroyContext        = EGLBoolean ( * )( EGLDisplay, EGLContext );
    using TEGLCreatePbufferSurface  = EGLSurface ( * )( EGLDisplay, EGLConfig, const EGLint * );
    using TEGLCreateWindowSurface   = EGLSurface ( * )( EGLDisplay, EGLConfig, EGLNativeDisplayType, const EGLint * );
    using TEGLDestroySurface        = EGLBoolean ( * )( EGLDisplay, EGLSurface );
    using TEGLMakeCurrent           = EGLBoolean ( * )( EGLDisplay, EGLSurface, EGLSurface, EGLContext );
    using TEGLSwapInterval          = EGLBoolean ( * )( EGLDisplay, EGLint );
    using TEGLSwapBuffers           = EGLBoolean ( * )( EGLDisplay, EGLSurface );
    using TEGLQueryString           = const AnsiChar *( * )( EGLDisplay, EGLint );
    using TEGLGetError              = EGLenum ( * )();

} // namespace RHIOpenGL

} // namespace Lumen
