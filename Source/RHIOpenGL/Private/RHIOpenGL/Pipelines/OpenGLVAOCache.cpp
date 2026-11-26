/**
 * @file OpenGLVAOCache.cpp
 * @brief Implements GL-thread DSA vertex array object creation and reuse.
 */

#include <RHIOpenGL/Pipelines/OpenGLVAOCache.hpp>

#include <Algo/Any.hpp>
#include <Algo/ForEach.hpp>

#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIFormatInfo.hpp>

#include <Templates/Vector.hpp>

#include <RHIOpenGL/Types/OpenGLFormatMapping.hpp>

/**
 * File-local helpers
 */

namespace
{

/** @return True when a fixed-point vertex attribute carries normalized data. */
[[nodiscard]] Lumen::Bool IsNormalizedVertexFormat ( const Lumen::RHI::ERHIFormat InFormat ) noexcept
{
    using Lumen::RHI::ERHIFormat;
    switch ( InFormat )
    {
    case ERHIFormat::R8Unorm:
    case ERHIFormat::R8Snorm:
    case ERHIFormat::RG8Unorm:
    case ERHIFormat::RG8Snorm:
    case ERHIFormat::RGBA8Unorm:
    case ERHIFormat::RGBA8Snorm:
    case ERHIFormat::BGRA8Unorm:
    case ERHIFormat::R16Unorm:
    case ERHIFormat::R16Snorm:
    case ERHIFormat::RG16Unorm:
    case ERHIFormat::RG16Snorm:
    case ERHIFormat::RGBA16Unorm:
    case ERHIFormat::RGBA16Snorm:
        return true;
    default:
        return false;
    }
}

} // namespace

/**
 * Public
 */

Lumen::RHIOpenGL::FOpenGLVAOCache::FOpenGLVAOCache ( TSharedRef<FOpenGLDeviceContext> InContext ) noexcept
    : Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLVAOCache::~FOpenGLVAOCache () noexcept
{
    if ( VAOs.empty() )
    {
        return;
    }
    TVector<GLuint> NativeVAOs;
    NativeVAOs.reserve( VAOs.size() );
    Algo::ForEach( VAOs, [&NativeVAOs] ( const auto &InPair ) noexcept
    {
        NativeVAOs.push_back( InPair.second );
    } );
    const FGLDispatchTable &Dispatch = Context->GetDispatch();
    ( void )Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, &NativeVAOs] () noexcept
    {
        Dispatch.DeleteVertexArrays( static_cast<GLsizei>( NativeVAOs.size() ), NativeVAOs.data() );
    } );
}

Lumen::TExpected<Lumen::RHIOpenGL::GLuint, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLVAOCache::GetOrCreate ( const RHI::FRHIVertexLayout &InLayout ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InLayout.Validate() );

    const UInt64 LayoutHash                          = InLayout.GetHash();
    const TMap<UInt64, GLuint>::const_iterator Found = VAOs.find( LayoutHash );
    if ( Found != VAOs.end() )
    {
        return Found->second;
    }

    const RHI::FRHILimits &Limits = Context->GetLimits();
    if ( InLayout.Buffers.size() > static_cast<USize>( Limits.MaxVertexBuffers ) or
         InLayout.Attributes.size() > static_cast<USize>( Limits.MaxVertexAttributes ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Vertex layout exceeds the advertised vertex input limits." ) );
    }
    if ( Algo::AnyOf( InLayout.Buffers, [&Limits] ( const RHI::FRHIVertexBufferLayout &InBuffer ) noexcept -> Bool
    {
        return InBuffer.Slot >= Limits.MaxVertexBuffers;
    } ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Vertex buffer slot exceeds the advertised binding limit." ) );
    }
    /* One pass: both failures are reported from the same attribute, so their original order is preserved. */
    for ( const RHI::FRHIVertexAttribute &Attribute : InLayout.Attributes )
    {
        if ( Attribute.Location >= Limits.MaxVertexAttributes or Attribute.BufferSlot >= Limits.MaxVertexBuffers )
        {
            return TUnexpected( RHI::MakeRHIInvalidLimit( "Vertex attribute exceeds the advertised input limits." ) );
        }
        const TOptional<FOpenGLFormatMapping> Mapping = MapOpenGLFormat( Attribute.Format );
        const RHI::FRHIFormatInfo &Info               = RHI::GetRHIFormatInfo( Attribute.Format );
        if ( not Mapping or Info.bIsCompressed or not Info.HasColor() )
        {
            return TUnexpected( RHI::MakeRHIInvalidPipeline( "Vertex attribute format has no OpenGL vertex mapping." ) );
        }
    }

    const FGLDispatchTable &Dispatch = Context->GetDispatch();
    GLuint NativeVAO                 = 0U;
    GLenum NativeError               = GlNoError;
    const RHI::FRHIExpected Created =
        Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, &InLayout, &NativeVAO, &NativeError] () noexcept
    {
        Dispatch.CreateVertexArrays( 1, &NativeVAO );
        if ( NativeVAO == 0U )
        {
            NativeError = Dispatch.GetError();
            return;
        }
        Dispatch.BindVertexArray( NativeVAO );
        Algo::ForEach( InLayout.Buffers, [&] ( const RHI::FRHIVertexBufferLayout &InBuffer ) noexcept
        {
            Dispatch.VertexArrayVertexBuffer( NativeVAO, InBuffer.Slot, 0U, 0, static_cast<GLsizei>( InBuffer.Stride ) );
            Dispatch.VertexArrayBindingDivisor( NativeVAO, InBuffer.Slot,
                                                InBuffer.Rate == RHI::ERHIVertexInputRate::Instance ? 1U : 0U );
        } );
        Algo::ForEach( InLayout.Attributes, [&] ( const RHI::FRHIVertexAttribute &InAttribute ) noexcept
        {
            const FOpenGLFormatMapping Mapping = *MapOpenGLFormat( InAttribute.Format );
            const GLboolean Normalized = static_cast<GLboolean>( IsNormalizedVertexFormat( InAttribute.Format ) ? 1U : 0U );
            Dispatch.VertexArrayAttribFormat( NativeVAO, InAttribute.Location, Mapping.Components, Mapping.Type, Normalized,
                                              InAttribute.Offset );
            Dispatch.VertexArrayAttribBinding( NativeVAO, InAttribute.Location, InAttribute.BufferSlot );
            Dispatch.EnableVertexArrayAttrib( NativeVAO, InAttribute.Location );
        } );
        NativeError = Dispatch.GetError();
    } );
    LUMEN_TRY_EXPECT_ERROR( Created );
    if ( NativeError != GlNoError or NativeVAO == 0U )
    {
        if ( NativeVAO != 0U )
        {
            ( void )Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeVAO] () noexcept
            {
                Dispatch.DeleteVertexArrays( 1, &NativeVAO );
            } );
        }
        return TUnexpected(
            RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Pipeline, "OpenGL refused vertex array object creation." ) );
    }

    VAOs.emplace( LayoutHash, NativeVAO );
    return NativeVAO;
}
