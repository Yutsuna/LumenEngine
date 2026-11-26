/**
 * @file OpenGLHeap.cpp
 * @brief Implements OpenGL buffer-backed heaps and persistent CPU mappings.
 */

#include <RHIOpenGL/Memory/OpenGLHeap.hpp>

#include <limits>

#include <RHI/Types/RHIError.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLHeap>, Lumen::RHI::FRHIError> Lumen::RHIOpenGL::FOpenGLHeap::Create (
    TSharedRef<FOpenGLDeviceContext> InContext, const RHI::FRHIHeapDesc &InDesc, const RHI::FRHIObjectId InId ) noexcept
{
    if ( const RHI::FRHIExpected Validated = InDesc.Validate(); not Validated )
    {
        return TUnexpected( Validated.Error() );
    }
    if ( InDesc.Size > static_cast<UInt64>( ( std::numeric_limits<GLsizeiptr>::max )() ) )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Limit, "Heap exceeds the GL buffer size range." ) );
    }

    TSharedRef<FOpenGLHeap> Heap =
        MakeShared<FOpenGLHeap>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    const FGLDispatchTable &Dispatch = Heap->Context->GetDispatch();
    const Bool bCpuVisible           = RHI::IsRHICpuAccessible( InDesc.MemoryClass );

    GLbitfield Flags = GlDynamicStorageBit;
    if ( InDesc.MemoryClass == RHI::ERHIMemoryClass::Upload )
    {
        Flags |= GlMapWriteBit | GlMapPersistentBit | GlMapCoherentBit;
    }
    else if ( InDesc.MemoryClass == RHI::ERHIMemoryClass::Readback )
    {
        Flags |= GlMapReadBit | GlMapPersistentBit | GlMapCoherentBit | GlClientStorageBit;
    }

    GLuint Buffer                   = 0U;
    Byte *Mapped                    = nullptr;
    GLenum NativeError              = GlNoError;
    const RHI::FRHIExpected Created = Heap->Context->GetExecutor()->ExecuteOnGLThread(
        [&Dispatch, &Buffer, &Mapped, &NativeError, InDesc, Flags, bCpuVisible] () noexcept
    {
        Dispatch.CreateBuffers( 1, &Buffer );
        if ( Buffer == 0U )
        {
            return;
        }
        Dispatch.NamedBufferStorage( Buffer, static_cast<GLsizeiptr>( InDesc.Size ), nullptr, Flags );
        NativeError = Dispatch.GetError();
        if ( NativeError != GlNoError )
        {
            return;
        }
        if ( bCpuVisible )
        {
            Mapped      = static_cast<Byte *>( Dispatch.MapNamedBufferRange(
                Buffer, 0, static_cast<GLsizeiptr>( InDesc.Size ),
                GlMapPersistentBit | GlMapCoherentBit |
                    ( InDesc.MemoryClass == RHI::ERHIMemoryClass::Upload ? GlMapWriteBit : GlMapReadBit ) ) );
            NativeError = Dispatch.GetError();
        }
    } );
    if ( not Created )
    {
        return TUnexpected( Created.Error() );
    }
    if ( NativeError != GlNoError or Buffer == 0U or ( bCpuVisible and Mapped == nullptr ) )
    {
        if ( Buffer != 0U )
        {
            ( void )Heap->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, Buffer] () noexcept
            {
                Dispatch.DeleteBuffers( 1, &Buffer );
            } );
        }
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Memory,
                                                        "OpenGL refused to allocate the heap buffer storage." ) );
    }

    Heap->Buffer = Buffer;
    Heap->Mapped = Mapped;
    return Heap;
}

Lumen::RHIOpenGL::FOpenGLHeap::~FOpenGLHeap () noexcept
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

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLHeap );

const Lumen::RHI::FRHIHeapDesc &Lumen::RHIOpenGL::FOpenGLHeap::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLHeap::Map ( const UInt64 InOffset, const UInt64 InSize ) noexcept
{
    if ( Mapped == nullptr or InSize == 0ULL )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Heap is not CPU accessible." ) );
    }
    if ( bMapped )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Memory, "Heap is already mapped." ) );
    }
    if ( not RHI::IsRHICpuAccessible( Desc.MemoryClass ) )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Device-local heaps cannot be mapped." ) );
    }
    const TOptional<UInt64> End = RHI::RHICheckedAdd( InOffset, InSize );
    if ( not End or *End > Desc.Size )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Heap mapping range is out of bounds." ) );
    }
    bMapped = true;
    return TSpan<Byte>( Mapped + static_cast<USize>( InOffset ), static_cast<USize>( InSize ) );
}

void Lumen::RHIOpenGL::FOpenGLHeap::Unmap () noexcept
{
    bMapped = false;
}

Lumen::RHIOpenGL::GLuint Lumen::RHIOpenGL::FOpenGLHeap::GetBuffer () const noexcept
{
    return Buffer;
}

Lumen::Byte *Lumen::RHIOpenGL::FOpenGLHeap::GetMappedBase () const noexcept
{
    return Mapped;
}

const Lumen::RHIOpenGL::FOpenGLDeviceContext &Lumen::RHIOpenGL::FOpenGLHeap::GetDeviceContext () const noexcept
{
    return *Context;
}

/**
 * Private
 */

Lumen::RHIOpenGL::FOpenGLHeap::FOpenGLHeap ( TSharedRef<FOpenGLDeviceContext> InContext,
                                             const RHI::FRHIHeapDesc &InDesc,
                                             const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLHeap>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc )
{
    /* */
}
