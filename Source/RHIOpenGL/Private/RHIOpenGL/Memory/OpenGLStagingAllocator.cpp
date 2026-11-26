/**
 * @file OpenGLStagingAllocator.cpp
 * @brief Implements persistent OpenGL upload and readback rings.
 */

#include <RHIOpenGL/Memory/OpenGLStagingAllocator.hpp>

#include <RHI/Memory/RHIAllocationDesc.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Types/RHIError.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLUploadAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLUploadAllocator::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                   const UInt64 InCapacity,
                                                   const RHI::FRHIObjectId InId,
                                                   const RHI::FRHIObjectId InBufferId ) noexcept
{
    const RHI::FRHIBufferDesc Desc{
        .Size = InCapacity, .Usage = RHI::ERHIResourceUsage::CopySource, .MemoryClass = RHI::ERHIMemoryClass::Upload };
    TExpected<TSharedRef<FOpenGLBuffer>, RHI::FRHIError> Buffer = FOpenGLBuffer::Create( InContext, Desc, InBufferId );
    LUMEN_TRY_EXPECT_ERROR( Buffer );
    return MakeShared<FOpenGLUploadAllocator>( static_cast<TSharedRef<FOpenGLBuffer> &&>( *Buffer ), InId );
}

Lumen::RHIOpenGL::FOpenGLUploadAllocator::FOpenGLUploadAllocator ( TSharedRef<FOpenGLBuffer> InBuffer,
                                                                   const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLUploadAllocator>( InId ) ),
      Buffer( static_cast<TSharedRef<FOpenGLBuffer> &&>( InBuffer ) ), Ring( InBuffer->GetDesc().Size )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLUploadAllocator::~FOpenGLUploadAllocator () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLUploadAllocator );

Lumen::TExpected<Lumen::RHI::FRHIUploadAllocation, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLUploadAllocator::Allocate ( const RHI::FRHIAllocationDesc &InDesc ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    TExpected<RHI::FRHIAllocation, RHI::FRHIError> Allocation = Ring.Allocate( InDesc );
    LUMEN_TRY_EXPECT_ERROR( Allocation );
    Allocation->MemoryClass = RHI::ERHIMemoryClass::Upload;
    Byte *Base              = Buffer->GetMappedBase();
    if ( Base == nullptr )
    {
        ( void )Ring.Free( *Allocation );
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Memory, "Upload buffer is not mapped." ) );
    }
    return RHI::FRHIUploadAllocation{
        .Allocation = *Allocation,
        .Buffer     = Buffer,
        .MappedData = TSpan<Byte>( Base + static_cast<USize>( Allocation->Offset ), static_cast<USize>( Allocation->Size ) ) };
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLUploadAllocator::Free ( const RHI::FRHIAllocation &InAllocation ) noexcept
{
    if ( not Owns( InAllocation ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Upload allocation is not owned by this allocator." ) );
    }
    return Ring.Free( InAllocation );
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLUploadAllocator::Owns ( const RHI::FRHIAllocation &InAllocation ) const noexcept
{
    const UInt64 Capacity = Buffer->GetDesc().Size;
    return InAllocation.IsValid() and InAllocation.MemoryClass == RHI::ERHIMemoryClass::Upload and
           InAllocation.Offset < Capacity and InAllocation.Size <= Capacity - InAllocation.Offset;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLReadbackAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLReadbackAllocator::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                     const UInt64 InCapacity,
                                                     const RHI::FRHIObjectId InId,
                                                     const RHI::FRHIObjectId InBufferId ) noexcept
{
    const RHI::FRHIBufferDesc Desc{
        .Size = InCapacity, .Usage = RHI::ERHIResourceUsage::CopyDestination, .MemoryClass = RHI::ERHIMemoryClass::Readback };
    TExpected<TSharedRef<FOpenGLBuffer>, RHI::FRHIError> Buffer = FOpenGLBuffer::Create( InContext, Desc, InBufferId );
    LUMEN_TRY_EXPECT_ERROR( Buffer );
    return MakeShared<FOpenGLReadbackAllocator>( static_cast<TSharedRef<FOpenGLBuffer> &&>( *Buffer ), InId );
}

Lumen::RHIOpenGL::FOpenGLReadbackAllocator::FOpenGLReadbackAllocator ( TSharedRef<FOpenGLBuffer> InBuffer,
                                                                       const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLReadbackAllocator>( InId ) ),
      Buffer( static_cast<TSharedRef<FOpenGLBuffer> &&>( InBuffer ) ), Ring( InBuffer->GetDesc().Size )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLReadbackAllocator::~FOpenGLReadbackAllocator () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLReadbackAllocator );

Lumen::TExpected<Lumen::RHI::FRHIAllocation, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLReadbackAllocator::Allocate ( const RHI::FRHIAllocationDesc &InDesc ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    TExpected<RHI::FRHIAllocation, RHI::FRHIError> Allocation = Ring.Allocate( InDesc );
    LUMEN_TRY_EXPECT_ERROR( Allocation );
    Allocation->MemoryClass = RHI::ERHIMemoryClass::Readback;
    return *Allocation;
}

Lumen::TExpected<Lumen::RHI::FRHIReadbackAllocation, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLReadbackAllocator::Map ( const RHI::FRHIAllocation &InAllocation ) noexcept
{
    if ( not Owns( InAllocation ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Readback allocation is not owned by this allocator." ) );
    }
    Byte *Base = Buffer->GetMappedBase();
    if ( Base == nullptr )
    {
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Memory, "Readback buffer is not mapped." ) );
    }
    return RHI::FRHIReadbackAllocation{ .Allocation = InAllocation,
                                        .MappedData = TSpan<const Byte>( Base + static_cast<USize>( InAllocation.Offset ),
                                                                         static_cast<USize>( InAllocation.Size ) ) };
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLReadbackAllocator::GetBuffer ( const RHI::FRHIAllocation &InAllocation ) noexcept
{
    if ( not Owns( InAllocation ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Readback allocation is not owned by this allocator." ) );
    }
    return Buffer;
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLReadbackAllocator::Free ( const RHI::FRHIAllocation &InAllocation ) noexcept
{
    return Ring.Free( InAllocation );
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLReadbackAllocator::Owns ( const RHI::FRHIAllocation &InAllocation ) const noexcept
{
    const UInt64 Capacity = Buffer->GetDesc().Size;
    return InAllocation.IsValid() and InAllocation.MemoryClass == RHI::ERHIMemoryClass::Readback and
           InAllocation.Offset < Capacity and InAllocation.Size <= Capacity - InAllocation.Offset;
}
