/**
 * @file OpenGLBindingSet.cpp
 * @brief Implements atomic validated writes for OpenGL binding sets.
 */

#include "Algo/Find.hpp"
#include <RHIOpenGL/Bindings/OpenGLBindingSet.hpp>

#include <HAL/LockGuard.hpp>

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/IRHISampler.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Validation/RHIValidation.hpp>

#include <RHIOpenGL/Resources/OpenGLBuffer.hpp>
#include <RHIOpenGL/Resources/OpenGLSampler.hpp>
#include <RHIOpenGL/Resources/OpenGLTexture.hpp>
#include <RHIOpenGL/Resources/OpenGLTextureView.hpp>

#include <variant>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLBindingSet>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLBindingSet::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                              const TSharedRef<RHI::IRHIBindingLayout> &InLayout,
                                              const RHI::FRHIObjectId InId ) noexcept
{
    TSharedRef<RHI::IRHIBindingLayout> UnwrappedLayout = RHI::UnwrapRHIShared( InLayout );
    if ( not RHI::TryRHIBackendCast<FOpenGLBindingLayout>( *UnwrappedLayout ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Binding layout does not belong to the OpenGL backend." ) );
    }
    return MakeShared<FOpenGLBindingSet>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ),
                                          static_cast<TSharedRef<RHI::IRHIBindingLayout> &&>( UnwrappedLayout ), InId );
}

Lumen::RHIOpenGL::FOpenGLBindingSet::FOpenGLBindingSet ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                         TSharedRef<RHI::IRHIBindingLayout> InLayout,
                                                         const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLBindingSet>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ),
      Layout( static_cast<TSharedRef<RHI::IRHIBindingLayout> &&>( InLayout ) )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLBindingSet::~FOpenGLBindingSet () noexcept = default;

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLBindingSet );

Lumen::TSharedRef<Lumen::RHI::IRHIBindingLayout> Lumen::RHIOpenGL::FOpenGLBindingSet::GetLayout () const noexcept
{
    return Layout;
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLBindingSet::Write ( const TSpan<const RHI::FRHIBindingWrite> InWrites ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( const RHI::FRHIBindingWrite &Write : InWrites )
    {
        LUMEN_TRY_EXPECT_ERROR( ValidateWrite( Write ) );
    }
    Writes.assign( InWrites.begin(), InWrites.end() );
    bDirty = true;
    return {};
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLBindingSet::IsDirty () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return bDirty;
}

void Lumen::RHIOpenGL::FOpenGLBindingSet::ClearDirty () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    bDirty = false;
}

const Lumen::TVector<Lumen::RHI::FRHIBindingWrite> &Lumen::RHIOpenGL::FOpenGLBindingSet::GetWrites () const noexcept
{
    return Writes;
}

const Lumen::RHIOpenGL::FOpenGLDeviceContext &Lumen::RHIOpenGL::FOpenGLBindingSet::GetDeviceContext () const noexcept
{
    return *Context;
}

/**
 * Private
 */

Lumen::Bool Lumen::RHIOpenGL::FOpenGLBindingSet::OwnsBuffer ( RHI::IRHIBuffer *InBuffer ) const noexcept
{
    const TOptional<FOpenGLBuffer *> Buffer = RHI::TryUnwrapRHIBackend<FOpenGLBuffer>( InBuffer );
    return Buffer and &( *Buffer )->GetDeviceContext() == Context.Get();
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLBindingSet::OwnsView ( RHI::IRHITextureView *InView ) const noexcept
{
    const TOptional<FOpenGLTextureView *> View = RHI::TryUnwrapRHIBackend<FOpenGLTextureView>( InView );
    return View and &( *View )->GetDeviceContext() == Context.Get();
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLBindingSet::OwnsSampler ( RHI::IRHISampler *InSampler ) const noexcept
{
    const TOptional<FOpenGLSampler *> Sampler = RHI::TryUnwrapRHIBackend<FOpenGLSampler>( InSampler );
    return Sampler and &( *Sampler )->GetDeviceContext() == Context.Get();
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLBindingSet::ValidateWrite ( const RHI::FRHIBindingWrite &InWrite ) const noexcept
{
    using RHI::ERHIBindingType;
    using RHI::ERHIResourceUsage;
    using RHI::FRHIError;
    using RHI::MakeRHIInvalidBinding;

    LUMEN_TRY_EXPECT_ERROR( InWrite.Validate() );

    const RHI::FRHIBindingLayoutEntry *Entry =
        Algo::FindIf( Layout->GetDesc().Entries, [&] ( const RHI::FRHIBindingLayoutEntry &Candidate )
    {
        return Candidate.Slot == InWrite.Slot;
    } );
    if ( Entry == nullptr )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Binding slot is absent from the layout." ) );
    }
    if ( InWrite.ArrayElement >= Entry->Count )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Binding array element exceeds the layout count." ) );
    }

    const Bool bValid = std::visit( [this, Entry] ( const auto &Payload ) noexcept -> Bool
    {
        if constexpr ( requires { Payload.Buffer; } )
        {
            const Bool bTypeMatches = Entry->Type == ERHIBindingType::ConstantBuffer or
                                      Entry->Type == ERHIBindingType::ReadOnlyBuffer or
                                      Entry->Type == ERHIBindingType::ReadWriteBuffer;
            if ( not bTypeMatches or not Payload.Buffer or not OwnsBuffer( Payload.Buffer.Get() ) )
            {
                return false;
            }
            ERHIResourceUsage Required = ERHIResourceUsage::ShaderResource;
            if ( Entry->Type == ERHIBindingType::ConstantBuffer )
            {
                Required = ERHIResourceUsage::ConstantBuffer;
            }
            else if ( Entry->Type == ERHIBindingType::ReadWriteBuffer )
            {
                Required = ERHIResourceUsage::UnorderedAccess;
            }
            const RHI::FRHIBufferDesc &BufferDesc = Payload.Buffer->GetDesc();
            return Payload.Offset <= BufferDesc.Size and Payload.Size <= BufferDesc.Size - Payload.Offset and
                   HasAnyFlags( BufferDesc.Usage, Required );
        }
        else if constexpr ( requires {
                                Payload.View;
                                Payload.Sampler;
                            } )
        {
            if ( Entry->Type != ERHIBindingType::CombinedTextureSampler or not Payload.View or not Payload.Sampler or
                 not OwnsView( Payload.View.Get() ) or not OwnsSampler( Payload.Sampler.Get() ) )
            {
                return false;
            }
            const TSharedPtr<RHI::IRHITexture> Texture = Payload.View->GetTexture();
            return Texture and HasAnyFlags( Texture->GetUsage(), ERHIResourceUsage::ShaderResource );
        }
        else if constexpr ( requires { Payload.View; } )
        {
            const Bool bTypeMatches = Entry->Type == ERHIBindingType::Texture or Entry->Type == ERHIBindingType::StorageTexture;
            if ( not bTypeMatches or not Payload.View or not OwnsView( Payload.View.Get() ) )
            {
                return false;
            }
            const TSharedPtr<RHI::IRHITexture> Texture = Payload.View->GetTexture();
            if ( not Texture )
            {
                return false;
            }
            const ERHIResourceUsage Required = Entry->Type == ERHIBindingType::StorageTexture ? ERHIResourceUsage::UnorderedAccess
                                                                                              : ERHIResourceUsage::ShaderResource;
            return HasAnyFlags( Texture->GetUsage(), Required );
        }
        else
        {
            return Entry->Type == ERHIBindingType::Sampler and Payload.Sampler and OwnsSampler( Payload.Sampler.Get() );
        }
    }, InWrite.Resource );

    if ( not bValid )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Binding resource does not match its layout entry." ) );
    }
    return {};
}
