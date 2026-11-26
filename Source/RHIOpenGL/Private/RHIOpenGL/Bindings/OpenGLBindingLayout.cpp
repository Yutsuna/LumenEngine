/**
 * @file OpenGLBindingLayout.cpp
 * @brief Implements the immutable OpenGL binding layout.
 */

#include <RHIOpenGL/Bindings/OpenGLBindingLayout.hpp>

#include <RHI/Types/RHIError.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLBindingLayout>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLBindingLayout::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                 const RHI::FRHIBindingLayoutDesc &InDesc,
                                                 const RHI::FRHIObjectId InId ) noexcept
{
    if ( const RHI::FRHIExpected Validated = InDesc.Validate(); not Validated )
    {
        return TUnexpected( Validated.Error() );
    }

    UInt64 DescriptorCount = 0ULL;
    for ( const RHI::FRHIBindingLayoutEntry &Entry : InDesc.Entries )
    {
        DescriptorCount += static_cast<UInt64>( Entry.Count );
        if ( Entry.bBindless and not InContext->GetCapabilities().Supports( RHI::ERHICapabilityFlags::BindlessResources ) )
        {
            return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Capability,
                                                         "Bindless resources are not available in the OpenGL backend." ) );
        }
    }
    if ( DescriptorCount > static_cast<UInt64>( InContext->GetLimits().MaxDescriptorsPerSet ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Binding layout exceeds the advertised descriptor limit." ) );
    }

    return MakeShared<FOpenGLBindingLayout>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
}

Lumen::RHIOpenGL::FOpenGLBindingLayout::FOpenGLBindingLayout ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                               const RHI::FRHIBindingLayoutDesc &InDesc,
                                                               const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLBindingLayout>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLBindingLayout::~FOpenGLBindingLayout () noexcept = default;

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLBindingLayout );

const Lumen::RHI::FRHIBindingLayoutDesc &Lumen::RHIOpenGL::FOpenGLBindingLayout::GetDesc () const noexcept
{
    return Desc;
}
