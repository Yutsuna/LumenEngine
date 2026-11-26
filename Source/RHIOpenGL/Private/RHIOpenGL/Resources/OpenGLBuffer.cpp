/**
 * @file OpenGLBuffer.cpp
 * @brief Implements OpenGL buffer storage, placement and persistent mapping.
 */

#include <RHIOpenGL/Resources/OpenGLBuffer.hpp>

#include <limits>

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLBuffer::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                          const RHI::FRHIBufferDesc &InDesc,
                                          const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    if ( InDesc.Size > InContext->GetLimits().MaxBufferSize )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Limit, "Buffer exceeds MaxBufferSize." ) );
    }

    TSharedRef<FOpenGLBuffer> Buffer =
        MakeShared<FOpenGLBuffer>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    LUMEN_TRY_EXPECT_ERROR( Buffer->AllocateStorage() );
    return Buffer;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLBuffer::CreatePlaced ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                const RHI::FRHIBufferDesc &InDesc,
                                                TSharedRef<RHI::IRHIHeap> InHeap,
                                                const RHI::FRHIAllocation &InAllocation,
                                                const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    LUMEN_TRY_EXPECT_ERROR( ValidatePlacement( *InContext, InDesc, *InHeap, InAllocation ) );

    TSharedRef<FOpenGLBuffer> Buffer =
        MakeShared<FOpenGLBuffer>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    const TOptional<FOpenGLHeap *> NativeHeap = RHI::TryRHIBackendCast<FOpenGLHeap>( *InHeap );
    if ( not NativeHeap )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed buffer heap is not an OpenGL heap." ) );
    }
    Buffer->ParentHeap = TOptional<TSharedRef<RHI::IRHIHeap>>( static_cast<TSharedRef<RHI::IRHIHeap> &&>( InHeap ) );
    Buffer->HeapOffset = InAllocation.Offset;
    Buffer->Mapped     = ( *NativeHeap )->GetMappedBase() == nullptr
                             ? nullptr
                             : ( *NativeHeap )->GetMappedBase() + static_cast<USize>( InAllocation.Offset );
    return Buffer;
}

Lumen::RHIOpenGL::FOpenGLBuffer::FOpenGLBuffer ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                 const RHI::FRHIBufferDesc &InDesc,
                                                 const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLBuffer>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc ),
      Requirements{ .Size                 = InDesc.Size,
                    .Alignment            = InContext->GetLimits().MinBufferAlignment,
                    .PreferredMemoryClass = TOptional<RHI::ERHIMemoryClass>( InDesc.MemoryClass ) }
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLBuffer::~FOpenGLBuffer () noexcept
{
    if ( Buffer == 0U )
    {
        return;
    }
    const GLuint NativeBuffer        = Buffer;
    const FGLDispatchTable &Dispatch = Context->GetDispatch();
    ( void )Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeBuffer] () noexcept
    {
        Dispatch.DeleteBuffers( 1, &NativeBuffer );
    } );
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLBuffer );

const Lumen::RHI::FRHIResourceRequirements &Lumen::RHIOpenGL::FOpenGLBuffer::GetRequirements () const noexcept
{
    return Requirements;
}

Lumen::RHI::ERHIResourceUsage Lumen::RHIOpenGL::FOpenGLBuffer::GetUsage () const noexcept
{
    return Desc.Usage;
}

Lumen::RHI::ERHIResourceState Lumen::RHIOpenGL::FOpenGLBuffer::GetInitialState () const noexcept
{
    return Desc.InitialState;
}

Lumen::RHI::FRHIResourceLifetime Lumen::RHIOpenGL::FOpenGLBuffer::GetLifetime () const noexcept
{
    return {};
}

const Lumen::RHI::FRHIBufferDesc &Lumen::RHIOpenGL::FOpenGLBuffer::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLBuffer::Map ( const UInt64 InOffset, const UInt64 InSize ) noexcept
{
    if ( not RHI::IsRHICpuAccessible( Desc.MemoryClass ) or Mapped == nullptr )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "OpenGL device-local buffers cannot be mapped." ) );
    }
    if ( InSize == 0ULL or bIsMapped )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Resource, "Buffer map range or state is invalid." ) );
    }
    const TOptional<UInt64> End = RHI::RHICheckedAdd( InOffset, InSize );
    if ( not End or *End > Desc.Size )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Buffer mapping range is out of bounds." ) );
    }
    bIsMapped = true;
    return TSpan<Byte>( Mapped + static_cast<USize>( InOffset ), static_cast<USize>( InSize ) );
}

void Lumen::RHIOpenGL::FOpenGLBuffer::Unmap () noexcept
{
    bIsMapped = false;
}

Lumen::RHIOpenGL::GLuint Lumen::RHIOpenGL::FOpenGLBuffer::GetGLBuffer () const noexcept
{
    return ParentHeap
        .AndThen( [] ( const TSharedRef<RHI::IRHIHeap> &InHeap ) noexcept -> TOptional<GLuint>
    {
        return RHI::TryRHIBackendCast<FOpenGLHeap>( *InHeap ).Transform( [] ( FOpenGLHeap *InNativeHeap ) noexcept -> GLuint
        {
            return InNativeHeap->GetBuffer();
        } );
    } ).ValueOr( Buffer );
}

Lumen::UInt64 Lumen::RHIOpenGL::FOpenGLBuffer::GetGLBufferOffset () const noexcept
{
    return HeapOffset;
}

Lumen::Byte *Lumen::RHIOpenGL::FOpenGLBuffer::GetMappedBase () const noexcept
{
    return Mapped;
}

const Lumen::RHIOpenGL::FOpenGLDeviceContext &Lumen::RHIOpenGL::FOpenGLBuffer::GetDeviceContext () const noexcept
{
    return *Context;
}

/**
 * Private
 */

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLBuffer::AllocateStorage () noexcept
{
    if ( Desc.Size > static_cast<UInt64>( ( std::numeric_limits<GLsizeiptr>::max )() ) )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Limit, "Buffer exceeds the GL storage range." ) );
    }

    GLbitfield Flags = GlDynamicStorageBit;
    if ( Desc.MemoryClass == RHI::ERHIMemoryClass::Upload )
    {
        Flags |= GlMapWriteBit | GlMapPersistentBit | GlMapCoherentBit;
    }
    else if ( Desc.MemoryClass == RHI::ERHIMemoryClass::Readback )
    {
        Flags |= GlMapReadBit | GlMapPersistentBit | GlMapCoherentBit | GlClientStorageBit;
    }

    const FGLDispatchTable &Dispatch = Context->GetDispatch();
    GLuint NativeBuffer              = 0U;
    Byte *NativeMapped               = nullptr;
    GLenum NativeError               = GlNoError;
    const RHI::FRHIExpected Created  = Context->GetExecutor()->ExecuteOnGLThread(
        [&Dispatch, &NativeBuffer, &NativeMapped, &NativeError, Size = Desc.Size, Flags] () noexcept
    {
        Dispatch.CreateBuffers( 1, &NativeBuffer );
        if ( NativeBuffer == 0U )
        {
            return;
        }
        Dispatch.NamedBufferStorage( NativeBuffer, static_cast<GLsizeiptr>( Size ), nullptr, Flags );
        NativeError = Dispatch.GetError();
        if ( NativeError != GlNoError )
        {
            return;
        }
        if ( ( Flags & GlMapPersistentBit ) != 0U )
        {
            NativeMapped = static_cast<Byte *>( Dispatch.MapNamedBufferRange(
                NativeBuffer, 0, static_cast<GLsizeiptr>( Size ),
                GlMapPersistentBit | GlMapCoherentBit | ( ( Flags & GlMapWriteBit ) != 0U ? GlMapWriteBit : GlMapReadBit ) ) );
            NativeError  = Dispatch.GetError();
        }
    } );
    if ( not Created )
    {
        return Created;
    }
    if ( NativeError != GlNoError or NativeBuffer == 0U or ( ( Flags & GlMapPersistentBit ) != 0U and NativeMapped == nullptr ) )
    {
        if ( NativeBuffer != 0U )
        {
            ( void )Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeBuffer] () noexcept
            {
                Dispatch.DeleteBuffers( 1, &NativeBuffer );
            } );
        }
        return TUnexpected(
            RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Resource, "OpenGL refused buffer allocation." ) );
    }

    Buffer = NativeBuffer;
    Mapped = NativeMapped;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLBuffer::ValidatePlacement ( const FOpenGLDeviceContext &InContext,
                                                                              const RHI::FRHIBufferDesc &InDesc,
                                                                              RHI::IRHIHeap &InHeap,
                                                                              const RHI::FRHIAllocation &InAllocation ) noexcept
{
    if ( not InAllocation.IsValid() )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed buffer allocation is invalid." ) );
    }
    if ( InAllocation.Heap != &InHeap )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed buffer allocation identifies another heap." ) );
    }
    if ( InAllocation.MemoryClass != InDesc.MemoryClass )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed buffer memory class does not match the allocation." ) );
    }
    if ( InDesc.Size > InAllocation.Size )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed buffer exceeds its allocation." ) );
    }

    const TOptional<FOpenGLHeap *> NativeHeap = RHI::TryRHIBackendCast<FOpenGLHeap>( InHeap );
    if ( not NativeHeap or &( *NativeHeap )->GetDeviceContext() != &InContext )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed buffer heap belongs to another OpenGL context." ) );
    }
    const TOptional<UInt64> End = RHI::RHICheckedAdd( InAllocation.Offset, InAllocation.Size );
    if ( not End or *End > ( *NativeHeap )->GetDesc().Size )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed buffer range exceeds its heap." ) );
    }
    const UInt64 MinAlignment       = InContext.GetLimits().MinBufferAlignment;
    const UInt64 RequiredAlignment  = InAllocation.Alignment > MinAlignment ? InAllocation.Alignment : MinAlignment;
    const TOptional<UInt64> Aligned = RHI::RHIAlignUp( InAllocation.Offset, RequiredAlignment );
    if ( not Aligned or *Aligned != InAllocation.Offset )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Placed buffer alignment is insufficient." ) );
    }
    return {};
}
