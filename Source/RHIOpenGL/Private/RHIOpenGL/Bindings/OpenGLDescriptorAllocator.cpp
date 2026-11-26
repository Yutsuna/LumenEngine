/**
 * @file OpenGLDescriptorAllocator.cpp
 * @brief Implements device-scoped descriptor pool tokens and allocation.
 */

#include <RHIOpenGL/Bindings/OpenGLDescriptorAllocator.hpp>

#include <RHI/Types/RHIError.hpp>

/**
 * File-local helpers
 */

namespace
{

/**
 * Pool tokens are process-wide integers, so a single backend must outlive every
 * allocator instance (the common base uses it from its own destructor).
 */
[[nodiscard]] Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLDescriptorPoolBackend> GetSharedBackend () noexcept
{
    static Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLDescriptorPoolBackend> Backend =
        Lumen::MakeShared<Lumen::RHIOpenGL::FOpenGLDescriptorPoolBackend>();
    return Backend;
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::UInt64, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDescriptorPoolBackend::CreatePool ( const UInt32 InCapacity ) noexcept
{
    if ( InCapacity == 0U )
    {
        return TUnexpected( RHI::MakeRHIInvalidBinding( "Descriptor pool capacity is zero." ) );
    }
    return NextPool++;
}

void Lumen::RHIOpenGL::FOpenGLDescriptorPoolBackend::ReleasePool ( const UInt64 ) noexcept
{
    /* Pool tokens are integers; the descriptor slots themselves are owned by the common base. */
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLDescriptorAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDescriptorAllocator::Create ( const RHI::FRHIDescriptorAllocatorDesc &InDesc,
                                                       const RHI::FRHIObjectId InId ) noexcept
{
    if ( const RHI::FRHIExpected Validated = InDesc.Validate(); not Validated )
    {
        return TUnexpected( Validated.Error() );
    }
    TSharedRef<FOpenGLDescriptorPoolBackend> Backend = GetSharedBackend();
    return MakeShared<FOpenGLDescriptorAllocator>( InDesc, *Backend,
                                                   static_cast<TSharedRef<FOpenGLDescriptorPoolBackend> &&>( Backend ), InId );
}

Lumen::RHIOpenGL::FOpenGLDescriptorAllocator::FOpenGLDescriptorAllocator (
    const RHI::FRHIDescriptorAllocatorDesc &InDesc,
    RHI::IRHIDescriptorPoolBackend &InBackend,
    TSharedRef<FOpenGLDescriptorPoolBackend> InBackendOwner,
    const RHI::FRHIObjectId InId ) noexcept
    : RHI::FRHIDescriptorAllocatorBase( InDesc, InBackend ), Identity( MakeOpenGLObjectBase<FOpenGLDescriptorAllocator>( InId ) ),
      BackendOwner( static_cast<TSharedRef<FOpenGLDescriptorPoolBackend> &&>( InBackendOwner ) )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLDescriptorAllocator::~FOpenGLDescriptorAllocator () noexcept = default;

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLDescriptorAllocator );
