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
    using GLint      = Int32;
    using GLint64    = Int64;
    using GLubyte    = Byte;
    using GLsizei    = Int32;
    using GLintptr   = Int64;
    using GLsizeiptr = Int64;
    using GLbitfield = UInt32;
    using GLfloat    = Float32;
    using GLboolean  = UInt8;

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
    inline constexpr GLenum GlMaxShaderStorageBufferBindings        = 0x90D3U;
    inline constexpr GLenum GlMaxImageUnits                         = 0x8B39U;
    inline constexpr GLenum GlMaxTextureMaxAnisotropy               = 0x84FFU;
    inline constexpr GLenum GlMaxSamples                            = 0x8D57U;
    inline constexpr GLenum GlLowerLeft                             = 0x8CA1U;
    inline constexpr GLenum GlZeroToOne                             = 0x935FU;
    inline constexpr GLenum GlDepthClamp                            = 0x864FU;
    inline constexpr GLenum GlTextureCubeMapSeamless                = 0x884FU;

    /* Buffer storage flags (core 4.6). */
    inline constexpr GLbitfield GlMapReadBit        = 0x0001U;
    inline constexpr GLbitfield GlMapWriteBit       = 0x0002U;
    inline constexpr GLbitfield GlMapPersistentBit  = 0x0040U;
    inline constexpr GLbitfield GlMapCoherentBit    = 0x0080U;
    inline constexpr GLbitfield GlDynamicStorageBit = 0x0100U;
    inline constexpr GLbitfield GlClientStorageBit  = 0x0200U;

    /* Texture targets and formats needed by immutable storage and views. */
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

    /* Sampler state (core 4.6 plus the ARB anisotropy constant). */
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

    using TGLGetString              = const GLubyte *( * )( GLenum );
    using TGLGetStringi             = const GLubyte *( * )( GLenum, GLuint );
    using TGLGetIntegerv            = void ( * )( GLenum, GLint * );
    using TGLGetInteger64v          = void ( * )( GLenum, GLint64 * );
    using TGLGetError               = GLenum ( * )();
    using TGLClipControl            = void ( * )( GLenum, GLenum );
    using TGLGetGraphicsResetStatus = GLenum ( * )();

    using TGLCreateBuffers               = void ( * )( GLsizei, GLuint * );
    using TGLDeleteBuffers               = void ( * )( GLsizei, const GLuint * );
    using TGLNamedBufferStorage          = void ( * )( GLuint, GLsizeiptr, const void *, GLbitfield );
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
    using TEGLDestroySurface        = EGLBoolean ( * )( EGLDisplay, EGLSurface );
    using TEGLMakeCurrent           = EGLBoolean ( * )( EGLDisplay, EGLSurface, EGLSurface, EGLContext );
    using TEGLQueryString           = const AnsiChar *( * )( EGLDisplay, EGLint );
    using TEGLGetError              = EGLenum ( * )();

} // namespace RHIOpenGL

} // namespace Lumen
