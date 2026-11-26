/**
 * @file OpenGLCapabilityProbe.cpp
 * @brief Implements adapter discovery from a live OpenGL 4.6 core context.
 */

#include <RHIOpenGL/Device/OpenGLCapabilityProbe.hpp>

#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIHash.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>

#include <Memory/Span.hpp>
#include <String/String.hpp>
#include <String/StringView.hpp>
#include <Templates/Optional.hpp>

#include <Maths/Macros.hpp>

/**
 * Private
 */

namespace
{

[[nodiscard]] Lumen::FString ReadGLString ( const Lumen::RHIOpenGL::FGLDispatchTable &InDispatch,
                                            const Lumen::RHIOpenGL::GLenum InName ) noexcept
{
    if ( const Lumen::RHIOpenGL::GLubyte *Value = InDispatch.GetString( InName ) )
    {
        return { reinterpret_cast<const Lumen::AnsiChar *>( Value ) };
    }
    return {};
}

[[nodiscard]] Lumen::Int32 ReadGLInteger ( const Lumen::RHIOpenGL::FGLDispatchTable &InDispatch,
                                           const Lumen::RHIOpenGL::GLenum InName ) noexcept
{
    Lumen::RHIOpenGL::GLint Value = 0;
    InDispatch.GetIntegerv( InName, &Value );
    return Value;
}

[[nodiscard]] Lumen::UInt64 ReadGLInteger64 ( const Lumen::RHIOpenGL::FGLDispatchTable &InDispatch,
                                              const Lumen::RHIOpenGL::GLenum InName ) noexcept
{
    Lumen::RHIOpenGL::GLint64 Value = 0;
    InDispatch.GetInteger64v( InName, &Value );
    return ( Value > 0 ) ? static_cast<Lumen::UInt64>( Value ) : 0ULL;
}

[[nodiscard]] Lumen::Bool HasGLExtension ( const Lumen::RHIOpenGL::FGLDispatchTable &InDispatch,
                                           const Lumen::FStringView InName ) noexcept
{
    Lumen::RHIOpenGL::GLint Count = 0;
    InDispatch.GetIntegerv( Lumen::RHIOpenGL::GlNumExtensions, &Count );
    for ( Lumen::RHIOpenGL::GLint Index = 0; Index < Count; ++Index )
    {
        const Lumen::RHIOpenGL::GLubyte *Extension =
            InDispatch.GetStringi( Lumen::RHIOpenGL::GlExtensions, static_cast<Lumen::RHIOpenGL::GLuint>( Index ) );
        if ( Extension != nullptr and Lumen::FStringView( reinterpret_cast<const Lumen::AnsiChar *>( Extension ) ) == InName )
        {
            return true;
        }
    }
    return false;
}

[[nodiscard]] Lumen::Bool HasOpenGL46 ( const Lumen::FString &InVersion ) noexcept
{
    const Lumen::USize Separator = InVersion.find( ' ' );
    const Lumen::FStringView Number( InVersion.data(), ( Separator == Lumen::FString::npos ) ? InVersion.size() : Separator );
    if ( Number.size() < 3U or Number[1] != '.' )
    {
        return false;
    }
    const Lumen::UInt32 Major = static_cast<Lumen::UInt32>( Number[0] - '0' );
    const Lumen::UInt32 Minor = static_cast<Lumen::UInt32>( Number[2] - '0' );
    return Major > 4U or ( Major == 4U and Minor >= 6U );
}

[[nodiscard]] Lumen::RHI::ERHIAdapterType ClassifyAdapter ( const Lumen::FStringView InRenderer ) noexcept
{
    if ( InRenderer.contains( "llvmpipe" ) or InRenderer.contains( "softpipe" ) or InRenderer.contains( "swrast" ) or
         InRenderer.contains( "swiftshader" ) or InRenderer.contains( "microsoft basic render" ) )
    {
        return Lumen::RHI::ERHIAdapterType::Software;
    }
    if ( InRenderer.contains( "intel" ) or InRenderer.contains( "amd radeon graphics" ) or InRenderer.contains( "vega" ) or
         InRenderer.contains( "mali" ) or InRenderer.contains( "adreno" ) )
    {
        return Lumen::RHI::ERHIAdapterType::Integrated;
    }
    return Lumen::RHI::ERHIAdapterType::Unknown;
}

[[nodiscard]] Lumen::UInt64 MakeAdapterId ( const Lumen::FStringView InVendor, const Lumen::FStringView InRenderer ) noexcept
{
    Lumen::FString Identity( "RHIOpenGL:" );
    Identity.append( InVendor );
    Identity.append( ":" );
    Identity.append( InRenderer );
    Identity.append( ":0" );

    const Lumen::Byte *IdentityData = reinterpret_cast<const Lumen::Byte *>( Identity.data() );
    const Lumen::UInt64 Hash = Lumen::RHI::RHIHashBytes( Lumen::TSpan<const Lumen::Byte>( IdentityData, Identity.size() ) );

    return ( Hash == 0ULL ) ? 1ULL : Hash;
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::ProbeOpenGLCapabilities ( RHI::FRHIAdapterInfo &OutInfo, const FGLDispatchTable &InDispatch ) noexcept
{
    const FString Version = ReadGLString( InDispatch, GlVersion );
    if ( not HasOpenGL46( Version ) )
    {
        return TUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Capability, "RHIOpenGL requires an OpenGL 4.6 core context." ) );
    }

    const FString Vendor   = ReadGLString( InDispatch, GlVendor );
    const FString Renderer = ReadGLString( InDispatch, GlRenderer );

    OutInfo.AdapterId   = MakeAdapterId( Vendor, Renderer );
    OutInfo.Name        = Renderer.empty() ? FString( "OpenGL Adapter" ) : Renderer;
    OutInfo.BackendName = "RHIOpenGL";
    OutInfo.Type        = ClassifyAdapter( Renderer );

    using EFlags                  = RHI::ERHICapabilityFlags;
    OutInfo.Capabilities.Features = EFlags::DynamicRendering | EFlags::TimelineSemaphores | EFlags::IndirectDraw |
                                    EFlags::MultiDrawIndirect | EFlags::OcclusionQueries | EFlags::DepthClamp |
                                    EFlags::Presentation;
    if ( HasGLExtension( InDispatch, "GL_ARB_texture_filter_anisotropic" ) or
         HasGLExtension( InDispatch, "GL_EXT_texture_filter_anisotropic" ) or
         ReadGLInteger( InDispatch, GlMaxTextureMaxAnisotropy ) >= 2 )
    {
        OutInfo.Capabilities.Features |= EFlags::AnisotropicFiltering;
    }
    OutInfo.Capabilities.ShaderFormats.push_back( RHI::ERHIShaderFormat::Source );

    const Bool bSupportsTimestamps = OutInfo.Capabilities.Supports( RHI::ERHICapabilityFlags::TimestampQueries );
    OutInfo.QueueFamilies.push_back( RHI::FRHIQueueFamilyInfo{ .Type                  = RHI::ERHIQueueType::Graphics,
                                                               .Count                 = 1U,
                                                               .bSupportsPresentation = true,
                                                               .bSupportsTimestamps   = bSupportsTimestamps } );
    OutInfo.QueueFamilies.push_back( RHI::FRHIQueueFamilyInfo{
        .Type = RHI::ERHIQueueType::Compute, .Count = 1U, .bSupportsTimestamps = bSupportsTimestamps } );
    OutInfo.QueueFamilies.push_back(
        RHI::FRHIQueueFamilyInfo{ .Type = RHI::ERHIQueueType::Copy, .Count = 1U, .bSupportsTimestamps = bSupportsTimestamps } );

    RHI::FRHILimits &Limits                 = OutInfo.Limits;
    const UInt64 MaxBlockSize               = ReadGLInteger64( InDispatch, GlMaxShaderStorageBlockSize );
    constexpr UInt64 MaxSupportedBufferSize = 2ULL * 1024ULL * 1024ULL * 1024ULL;
    Limits.MaxBufferSize                    = 0ULL;
    if ( MaxBlockSize > 0ULL )
    {
        Limits.MaxBufferSize = MaxBlockSize;
        Limits.MaxBufferSize = Maths::Min( Limits.MaxBufferSize, MaxSupportedBufferSize );
    }
    Limits.MaxTextureDimension1D   = static_cast<UInt32>( ReadGLInteger( InDispatch, GlMaxTextureSize ) );
    Limits.MaxTextureDimension2D   = Limits.MaxTextureDimension1D;
    Limits.MaxTextureDimension3D   = static_cast<UInt32>( ReadGLInteger( InDispatch, GlMax3DTextureSize ) );
    Limits.MaxTextureArrayLayers   = static_cast<UInt32>( ReadGLInteger( InDispatch, GlMaxArrayTextureLayers ) );
    Limits.MaxColorAttachments     = static_cast<UInt32>( ReadGLInteger( InDispatch, GlMaxColorAttachments ) );
    Limits.MaxBindingSets          = 8U;
    Limits.MaxDescriptorsPerSet    = 32U;
    Limits.MaxInlineConstantBytes  = 256U;
    Limits.MaxVertexAttributes     = static_cast<UInt32>( ReadGLInteger( InDispatch, GlMaxVertexAttribs ) );
    Limits.MaxVertexBuffers        = static_cast<UInt32>( ReadGLInteger( InDispatch, GlMaxVertexAttribBindings ) );
    Limits.MaxComputeWorkgroupSize = static_cast<UInt32>( ReadGLInteger( InDispatch, GlMaxComputeWorkGroupInvocations ) );
    Limits.MinBufferAlignment      = static_cast<UInt64>( ReadGLInteger( InDispatch, GlMaxUniformBufferOffsetAlignment ) );
    const UInt64 StorageAlignment  = static_cast<UInt64>( ReadGLInteger( InDispatch, GlMaxShaderStorageBufferOffsetAlignment ) );
    Limits.MinBufferAlignment      = Maths::Max( StorageAlignment, Limits.MinBufferAlignment );
    if ( Limits.MinBufferAlignment == 0ULL or not RHI::RHIIsPowerOfTwo( Limits.MinBufferAlignment ) )
    {
        Limits.MinBufferAlignment = 256ULL;
    }
    Limits.MinTextureRowAlignment = 1ULL;
    Limits.TimestampPeriod        = 1ULL;

    LUMEN_TRY_EXPECT_ERROR( OutInfo.Validate() );
    return {};
}
