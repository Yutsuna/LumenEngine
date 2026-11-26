/**
 * @file OpenGLTexture.cpp
 * @brief Implements immutable OpenGL texture storage and byte accounting.
 */

#include <RHIOpenGL/Resources/OpenGLTexture.hpp>

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIFormatInfo.hpp>
#include <RHI/Types/RHISampleCount.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

#include <RHIOpenGL/Memory/OpenGLHeap.hpp>
#include <RHIOpenGL/Types/OpenGLFormatMapping.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLTexture>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLTexture::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                           const RHI::FRHITextureDesc &InDesc,
                                           const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    LUMEN_TRY_EXPECT_ERROR( ValidateLimits( *InContext, InDesc ) );

    TSharedRef<FOpenGLTexture> Texture =
        MakeShared<FOpenGLTexture>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    LUMEN_TRY_EXPECT_ERROR( Texture->AllocateStorage() );
    return Texture;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLTexture>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLTexture::CreatePlaced ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                 const RHI::FRHITextureDesc &InDesc,
                                                 TSharedRef<RHI::IRHIHeap> InHeap,
                                                 const RHI::FRHIAllocation &InAllocation,
                                                 const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    LUMEN_TRY_EXPECT_ERROR( ValidateLimits( *InContext, InDesc ) );
    if ( not InAllocation.IsValid() or InAllocation.Heap != InHeap.Get() )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed texture allocation does not identify the supplied heap." ) );
    }
    if ( InAllocation.MemoryClass != InDesc.MemoryClass )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed texture memory class does not match the allocation." ) );
    }
    const TOptional<FOpenGLHeap *> NativeHeap = RHI::TryRHIBackendCast<FOpenGLHeap>( *InHeap );
    if ( not NativeHeap or &( *NativeHeap )->GetDeviceContext() != InContext.Get() )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed texture heap belongs to another OpenGL context." ) );
    }
    const TOptional<UInt64> End = RHI::RHICheckedAdd( InAllocation.Offset, InAllocation.Size );
    if ( not End or *End > ( *NativeHeap )->GetDesc().Size )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed texture range exceeds its heap." ) );
    }
    const UInt64 MinAlignment       = InContext->GetLimits().MinBufferAlignment;
    const UInt64 RequiredAlignment  = InAllocation.Alignment > MinAlignment ? InAllocation.Alignment : MinAlignment;
    const TOptional<UInt64> Aligned = RHI::RHIAlignUp( InAllocation.Offset, RequiredAlignment );
    if ( not Aligned or *Aligned != InAllocation.Offset )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed texture alignment is insufficient." ) );
    }
    const RHI::FRHIFormatInfo &FormatInfo = RHI::GetRHIFormatInfo( InDesc.Format );
    UInt64 RequiredBytes                  = 0ULL;
    Bool bRequirementsValid = FormatInfo.BytesPerBlock > 0U and FormatInfo.BlockWidth > 0U and FormatInfo.BlockHeight > 0U;
    for ( UInt32 Mip = 0U; bRequirementsValid and Mip < InDesc.MipCount; ++Mip )
    {
        const UInt64 MipWidth  = InDesc.Extent.Width >> Mip;
        const UInt64 MipHeight = InDesc.Extent.Height >> Mip;
        const UInt64 MipDepth  = ( InDesc.Extent.Depth >> Mip ) < 1U ? 1ULL : static_cast<UInt64>( InDesc.Extent.Depth >> Mip );
        const TOptional<UInt64> BlocksWide = RHI::RHICheckedDivideCeil( MipWidth, static_cast<UInt64>( FormatInfo.BlockWidth ) );
        const TOptional<UInt64> BlocksHigh =
            RHI::RHICheckedDivideCeil( MipHeight, static_cast<UInt64>( FormatInfo.BlockHeight ) );
        const TOptional<UInt64> RowBytes =
            BlocksWide ? RHI::RHICheckedMultiply( *BlocksWide, static_cast<UInt64>( FormatInfo.BytesPerBlock ) )
                       : TOptional<UInt64>{};
        const TOptional<UInt64> PlaneBytes = RowBytes ? RHI::RHICheckedMultiply( *RowBytes, *BlocksHigh ) : TOptional<UInt64>{};
        const TOptional<UInt64> LevelBytes = PlaneBytes ? RHI::RHICheckedMultiply( *PlaneBytes, MipDepth ) : TOptional<UInt64>{};
        const TOptional<UInt64> TotalBytes = LevelBytes ? RHI::RHICheckedAdd( RequiredBytes, *LevelBytes ) : TOptional<UInt64>{};
        if ( not TotalBytes )
        {
            bRequirementsValid = false;
            break;
        }
        RequiredBytes = *TotalBytes;
    }
    if ( not bRequirementsValid or RequiredBytes > InAllocation.Size )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed texture exceeds its allocation." ) );
    }

    TSharedRef<FOpenGLTexture> Texture =
        MakeShared<FOpenGLTexture>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    LUMEN_TRY_EXPECT_ERROR( Texture->AllocateStorage() );
    Texture->ParentHeap      = TOptional<TSharedRef<RHI::IRHIHeap>>( static_cast<TSharedRef<RHI::IRHIHeap> &&>( InHeap ) );
    Texture->PlacementOffset = InAllocation.Offset;
    return Texture;
}

Lumen::TOptional<Lumen::UInt64>
Lumen::RHIOpenGL::FOpenGLTexture::ComputeTextureRequirements ( const RHI::FRHITextureDesc &InDesc,
                                                               const UInt64 InMinAlignment ) noexcept
{
    const RHI::FRHIFormatInfo &Info = RHI::GetRHIFormatInfo( InDesc.Format );
    if ( Info.BytesPerBlock == 0U or InDesc.Extent.Width == 0U or InDesc.Extent.Height == 0U or InDesc.Extent.Depth == 0U )
    {
        return {};
    }
    const UInt64 Alignment = InMinAlignment > 4096ULL ? InMinAlignment : 4096ULL;
    UInt64 Total           = 0ULL;
    for ( UInt32 Mip = 0U; Mip < InDesc.MipCount; ++Mip )
    {
        const UInt64 Width  = InDesc.Extent.Width >> Mip;
        const UInt64 Height = InDesc.Extent.Height >> Mip;
        const UInt64 Depth  = InDesc.Extent.Depth >> Mip;
        LUMEN_OPTIONAL( BlocksWide,
                        RHI::RHICheckedDivideCeil( Width < 1ULL ? 1ULL : Width, static_cast<UInt64>( Info.BlockWidth ) ) );
        LUMEN_OPTIONAL( BlocksHigh,
                        RHI::RHICheckedDivideCeil( Height < 1ULL ? 1ULL : Height, static_cast<UInt64>( Info.BlockHeight ) ) );
        LUMEN_OPTIONAL( LevelBytes, RHI::RHICheckedMultiply( BlocksWide, static_cast<UInt64>( Info.BytesPerBlock ) )
                                        .AndThen(
                                            [&Alignment] ( const UInt64 InRowBytes ) noexcept -> TOptional<UInt64>
        {
            return RHI::RHIAlignUp( InRowBytes, Alignment );
        } )
                                        .AndThen(
                                            [&BlocksHigh] ( const UInt64 InAlignedRow ) noexcept -> TOptional<UInt64>
        {
            return RHI::RHICheckedMultiply( InAlignedRow, BlocksHigh );
        } )
                                        .AndThen( [Depth] ( const UInt64 InPlaneBytes ) noexcept -> TOptional<UInt64>
        {
            return RHI::RHICheckedMultiply( InPlaneBytes, Depth < 1ULL ? 1ULL : Depth );
        } ) );
        LUMEN_OPTIONAL( Sum, RHI::RHICheckedAdd( Total, LevelBytes ) );
        Total = Sum;
    }
    return TOptional<UInt64>( Total );
}

Lumen::RHIOpenGL::FOpenGLTexture::FOpenGLTexture ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                   const RHI::FRHITextureDesc &InDesc,
                                                   const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLTexture>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc ),
      Requirements{ .Size = [&] () noexcept -> UInt64
{
    return ComputeTextureRequirements( InDesc, InContext->GetLimits().MinBufferAlignment ).ValueOr( 0ULL );
}(),
                    .Alignment            = InContext->GetLimits().MinBufferAlignment,
                    .PreferredMemoryClass = TOptional<RHI::ERHIMemoryClass>( InDesc.MemoryClass ) }
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLTexture::~FOpenGLTexture () noexcept
{
    if ( Texture == 0U )
    {
        return;
    }
    const GLuint NativeTexture       = Texture;
    const FGLDispatchTable &Dispatch = Context->GetDispatch();
    ( void )Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeTexture] () noexcept
    {
        Dispatch.DeleteTextures( 1, &NativeTexture );
    } );
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLTexture );

const Lumen::RHI::FRHIResourceRequirements &Lumen::RHIOpenGL::FOpenGLTexture::GetRequirements () const noexcept
{
    return Requirements;
}

Lumen::RHI::ERHIResourceUsage Lumen::RHIOpenGL::FOpenGLTexture::GetUsage () const noexcept
{
    return Desc.Usage;
}

Lumen::RHI::ERHIResourceState Lumen::RHIOpenGL::FOpenGLTexture::GetInitialState () const noexcept
{
    return Desc.InitialState;
}

Lumen::RHI::FRHIResourceLifetime Lumen::RHIOpenGL::FOpenGLTexture::GetLifetime () const noexcept
{
    return {};
}

const Lumen::RHI::FRHITextureDesc &Lumen::RHIOpenGL::FOpenGLTexture::GetDesc () const noexcept
{
    return Desc;
}

Lumen::RHIOpenGL::GLuint Lumen::RHIOpenGL::FOpenGLTexture::GetTexture () const noexcept
{
    return Texture;
}

Lumen::RHIOpenGL::GLenum Lumen::RHIOpenGL::FOpenGLTexture::GetTarget () const noexcept
{
    return Target;
}

const Lumen::RHIOpenGL::FOpenGLDeviceContext &Lumen::RHIOpenGL::FOpenGLTexture::GetDeviceContext () const noexcept
{
    return *Context;
}

/**
 * Private
 */

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLTexture::AllocateStorage () noexcept
{
    const TOptional<FOpenGLFormatMapping> Mapping = MapOpenGLFormat( Desc.Format );
    if ( not Mapping or Mapping->bCompressed )
    {
        return TUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Resource, "RHI format has no GL 4.6 storage mapping." ) );
    }

    const UInt32 Samples = RHI::ToSampleCount( Desc.SampleCount );
    if ( Desc.SampleCount != RHI::ERHISampleCount::Count1 )
    {
        if ( Desc.MipCount > 1U or Desc.Extent.Depth > 1U )
        {
            return TUnexpected( RHI::MakeRHIInvalidResource( "Multisample textures cannot have mips or depth." ) );
        }
        Target = Desc.LayerCount > 1U ? GlTexture2DMultisampleArray : GlTexture2DMultisample;
    }
    else if ( Desc.Extent.Depth > 1U )
    {
        Target = GlTexture3D;
    }
    else if ( Desc.LayerCount > 1U )
    {
        Target = GlTexture2DArray;
    }
    else
    {
        Target = GlTexture2D;
    }

    const FGLDispatchTable &Dispatch = Context->GetDispatch();
    GLuint NativeTexture             = 0U;
    GLenum NativeError               = GlNoError;
    const RHI::FRHIExpected Created =
        Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, &NativeTexture, &NativeError, this, Mapping, Samples] () noexcept
    {
        Dispatch.CreateTextures( Target, 1, &NativeTexture );
        if ( NativeTexture == 0U )
        {
            return;
        }
        if ( Target == GlTexture2DMultisample or Target == GlTexture2DMultisampleArray )
        {
            Dispatch.TextureStorage2DMultisample( NativeTexture, static_cast<GLsizei>( Samples ), Mapping->InternalFormat,
                                                  static_cast<GLsizei>( Desc.Extent.Width ),
                                                  static_cast<GLsizei>( Desc.Extent.Height ), 1U );
        }
        else if ( Target == GlTexture3D )
        {
            Dispatch.TextureStorage3D( NativeTexture, static_cast<GLsizei>( Desc.MipCount ), Mapping->InternalFormat,
                                       static_cast<GLsizei>( Desc.Extent.Width ), static_cast<GLsizei>( Desc.Extent.Height ),
                                       static_cast<GLsizei>( Desc.Extent.Depth ) );
        }
        else if ( Target == GlTexture2DArray )
        {
            Dispatch.TextureStorage3D( NativeTexture, static_cast<GLsizei>( Desc.MipCount ), Mapping->InternalFormat,
                                       static_cast<GLsizei>( Desc.Extent.Width ), static_cast<GLsizei>( Desc.Extent.Height ),
                                       static_cast<GLsizei>( Desc.LayerCount ) );
        }
        else
        {
            Dispatch.TextureStorage2D( NativeTexture, static_cast<GLsizei>( Desc.MipCount ), Mapping->InternalFormat,
                                       static_cast<GLsizei>( Desc.Extent.Width ), static_cast<GLsizei>( Desc.Extent.Height ) );
        }
        NativeError = Dispatch.GetError();
    } );
    if ( not Created )
    {
        return Created;
    }
    if ( NativeError != GlNoError or NativeTexture == 0U )
    {
        if ( NativeTexture != 0U )
        {
            ( void )Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeTexture] () noexcept
            {
                Dispatch.DeleteTextures( 1, &NativeTexture );
            } );
        }
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Resource, "OpenGL refused texture storage." ) );
    }
    Texture = NativeTexture;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLTexture::ValidateLimits ( const FOpenGLDeviceContext &InContext,
                                                                            const RHI::FRHITextureDesc &InDesc ) noexcept
{
    const RHI::FRHILimits &Limits = InContext.GetLimits();
    if ( InDesc.Extent.Width > Limits.MaxTextureDimension2D or InDesc.Extent.Height > Limits.MaxTextureDimension2D or
         InDesc.Extent.Depth > Limits.MaxTextureDimension3D or InDesc.LayerCount > Limits.MaxTextureArrayLayers )
    {
        return TUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Limit, "Texture exceeds a published dimension limit." ) );
    }
    if ( RHI::ToSampleCount( InDesc.SampleCount ) == 0U )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Invalid texture sample count." ) );
    }
    return {};
}
