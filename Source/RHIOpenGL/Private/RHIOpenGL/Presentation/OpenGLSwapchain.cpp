/**
 * @file OpenGLSwapchain.cpp
 * @brief Implements the bounded GL texture ring and surface-bridge presentation.
 */

#include <RHIOpenGL/Presentation/OpenGLSwapchain.hpp>

#include <Algo/Any.hpp>

#include <HAL/LockGuard.hpp>

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Presentation/OpenGLSurfaceBridge.hpp>
#include <RHIOpenGL/Resources/OpenGLTexture.hpp>

#include <Templates/Optional.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLSwapchain>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLSwapchain::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                             const RHI::FRHISwapchainDesc &InDesc,
                                             const RHI::FRHISwapchainCapabilities &InCapabilities,
                                             const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( InContext->GetUnavailableError( "Swapchain::Create" ) );
    LUMEN_EXPECT( Validated, InDesc.Validate() );
    LUMEN_EXPECT( Negotiated, InDesc.Validate( InCapabilities ) );
    if ( InDesc.PresentMode == RHI::ERHIPresentMode::Mailbox )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Presentation,
                                                     "Mailbox presentation is not available in OpenGL." ) );
    }

    TSharedRef<FOpenGLSwapchain> Swapchain = MakeShared<FOpenGLSwapchain>(
        static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InCapabilities, InId );
    if ( InDesc.Surface.Type == RHI::ERHISurfaceType::Window and InDesc.Surface.Bridge )
    {
        if ( FOpenGLSurfaceBridge *const NativeBridge =
                 dynamic_cast<FOpenGLSurfaceBridge *>( InDesc.Surface.Bridge.Get() ) )
        {
            NativeBridge->AttachContext( Swapchain->Context->GetExecutor() );
        }
        LUMEN_EXPECT( Surface, InDesc.Surface.Bridge->CreateNativeSurface( InDesc.Surface.NativeWindow ) );
        Swapchain->NativeSurface = TOptional<RHI::FRHINativeSurface>( Surface );
    }
    LUMEN_TRY_EXPECT_ERROR( Swapchain->RebuildImages() );
    return Swapchain;
}

Lumen::RHIOpenGL::FOpenGLSwapchain::FOpenGLSwapchain ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                       const RHI::FRHISwapchainDesc &InDesc,
                                                       const RHI::FRHISwapchainCapabilities &InCapabilities,
                                                       const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLSwapchain>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc ), Capabilities( InCapabilities )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLSwapchain::~FOpenGLSwapchain () noexcept
{
    if ( PresentationFramebuffer != 0U )
    {
        const GLuint Framebuffer = PresentationFramebuffer;
        ( void )Context->GetExecutor()->ExecuteOnGLThread( [this, Framebuffer] () noexcept
        {
            Context->GetDispatch().DeleteFramebuffers( 1, &Framebuffer );
        } );
    }
    if ( NativeSurface and Desc.Surface.Bridge )
    {
        Desc.Surface.Bridge->DestroyNativeSurface( *NativeSurface );
    }
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLSwapchain )

const Lumen::RHI::FRHISwapchainDesc &Lumen::RHIOpenGL::FOpenGLSwapchain::GetDesc () const noexcept
{
    return Desc;
}

const Lumen::RHI::FRHISwapchainCapabilities &Lumen::RHIOpenGL::FOpenGLSwapchain::GetCapabilities () const noexcept
{
    return Capabilities;
}

Lumen::UInt32 Lumen::RHIOpenGL::FOpenGLSwapchain::GetImageCount () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return static_cast<UInt32>( Slots.size() );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLSwapchain::GetImage ( const UInt32 InIndex ) const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( InIndex >= Slots.size() )
    {
        return TUnexpected( RHI::MakeRHIInvalidPresentation( "Swapchain image index is out of range." ) );
    }
    return Slots[InIndex].Image;
}

Lumen::TExpected<Lumen::RHI::FRHIAcquiredImage, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLSwapchain::AcquireNextImage () noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Swapchain::AcquireNextImage" ) );
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( bOutOfDate )
    {
        return TUnexpected( RHI::MakeRHIOutOfDate( RHI::ERHIErrorCondition::Presentation, "OpenGL swapchain is out of date." ) );
    }
    const USize Count = Slots.size();
    if ( Count == 0U )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Presentation, "OpenGL swapchain has no images." ) );
    }
    USize Selected = Count;
    for ( USize Offset = 0U; Offset < Count; ++Offset )
    {
        const USize Candidate = ( Cursor + Offset ) % Count;
        if ( not Slots[Candidate].bAcquired )
        {
            Selected = Candidate;
            break;
        }
    }
    if ( Selected == Count )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Presentation, "Every swapchain image is already acquired." ) );
    }

    FSlot &Slot = Slots[Selected];
    if ( Slot.PresentSemaphore->GetValue() != 0ULL )
    {
        LUMEN_TRY_EXPECT_ERROR( Slot.PresentSemaphore->Wait( 1ULL, 0.0 ) );
    }
    LUMEN_TRY_EXPECT_ERROR( Slot.AcquireSemaphore->Signal( 1ULL ) );

    Slot.bAcquired = true;
    Cursor         = static_cast<UInt32>( ( Selected + 1U ) % Count );
    RHI::FRHIAcquiredImage Result;
    Result.ImageIndex                               = static_cast<UInt32>( Selected );
    Result.Image                                    = Slot.Image;
    const TSharedRef<RHI::IRHISemaphore> AcquireRef = Slot.AcquireSemaphore;
    const TSharedRef<RHI::IRHISemaphore> PresentRef = Slot.PresentSemaphore;
    Result.AcquireSemaphore                         = AcquireRef;
    Result.PresentSemaphore                         = PresentRef;
    Result.bSuboptimal                              = bSuboptimal;
    bSuboptimal                                     = false;
    return Result;
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLSwapchain::Present ( RHI::IRHIQueue &InQueue,
                                                                       const RHI::FRHIAcquiredImage &InImage,
                                                                       RHI::IRHISemaphore *InRenderComplete ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Swapchain::Present" ) );
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( InQueue.GetType() != RHI::ERHIQueueType::Graphics or InImage.ImageIndex >= Slots.size() or not InImage.Image or
         not InImage.AcquireSemaphore or not InImage.PresentSemaphore )
    {
        return TUnexpected( RHI::MakeRHIInvalidPresentation( "Invalid presentation queue or acquired image." ) );
    }

    const USize Index = static_cast<USize>( InImage.ImageIndex );
    FSlot &Slot       = Slots[Index];
    if ( not Slot.bAcquired or InImage.Image != Slot.Image or InImage.AcquireSemaphore != Slot.AcquireSemaphore or
         InImage.PresentSemaphore != Slot.PresentSemaphore )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Presentation,
                                                      "Acquired image token is stale or already presented." ) );
    }
    /* Check readiness without consuming the binary semaphores: PresentToBridge() may still fail. */
    if ( InRenderComplete != nullptr )
    {
        if ( InRenderComplete->GetDesc().Type != RHI::ERHISemaphoreType::Binary )
        {
            return TUnexpected( RHI::MakeRHIInvalidPresentation( "The render-complete semaphore must be binary." ) );
        }
        if ( InRenderComplete->GetValue() < 1ULL )
        {
            return TUnexpected( RHI::MakeRHITimeout( RHI::ERHIErrorCondition::Synchronization,
                                                     "The render-complete semaphore is not signaled." ) );
        }
    }
    const Bool bWaitAcquire = Slot.AcquireSemaphore->GetValue() >= 1ULL;
    if ( bOutOfDate )
    {
        RHI::FRHIExpected Released{};
        if ( InRenderComplete != nullptr )
        {
            Released = InRenderComplete->Wait( 1ULL, 0.0 );
        }
        if ( Released and bWaitAcquire )
        {
            Released = Slot.AcquireSemaphore->Wait( 1ULL, 0.0 );
        }
        const RHI::FRHIExpected Signaled = Slot.PresentSemaphore->Signal( 1ULL );
        Slot.bAcquired                   = false;
        if ( not Released )
        {
            return Released;
        }
        if ( not Signaled )
        {
            return Signaled;
        }
        return TUnexpected( RHI::MakeRHIOutOfDate( RHI::ERHIErrorCondition::Presentation, "OpenGL swapchain is out of date." ) );
    }

    LUMEN_TRY_EXPECT_ERROR( PresentToBridge( *InImage.Image ) );

    if ( InRenderComplete != nullptr )
    {
        LUMEN_TRY_EXPECT_ERROR( InRenderComplete->Wait( 1ULL, 0.0 ) );
    }
    if ( bWaitAcquire )
    {
        LUMEN_TRY_EXPECT_ERROR( Slot.AcquireSemaphore->Wait( 1ULL, 0.0 ) );
    }
    LUMEN_TRY_EXPECT_ERROR( Slot.PresentSemaphore->Signal( 1ULL ) );
    Slot.bAcquired = false;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLSwapchain::Resize ( const RHI::FRHIExtent3D &InExtent ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Swapchain::Resize" ) );
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( Algo::AnyOf( Slots, [] ( const FSlot &InSlot ) noexcept -> Bool
    {
        return InSlot.bAcquired;
    } ) )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Presentation, "Cannot resize while an image is acquired." ) );
    }

    RHI::FRHISwapchainDesc NewDesc = Desc;
    NewDesc.Extent                 = InExtent;
    NewDesc.Surface.InitialExtent  = InExtent;
    LUMEN_EXPECT( Validated, NewDesc.Validate( Capabilities ) );

    const RHI::FRHISwapchainDesc Previous = Desc;
    Desc                                  = NewDesc;
    const RHI::FRHIExpected Rebuilt       = RebuildImages();
    if ( not Rebuilt )
    {
        Desc = Previous;
    }
    else
    {
        bOutOfDate = false;
        Cursor     = 0U;
        if ( FOpenGLSurfaceBridge *const NativeBridge = dynamic_cast<FOpenGLSurfaceBridge *>( Desc.Surface.Bridge.Get() ) )
        {
            NativeBridge->SetExtent( InExtent );
        }
    }
    return Rebuilt;
}

void Lumen::RHIOpenGL::FOpenGLSwapchain::MarkOutOfDate () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    bOutOfDate = true;
}

/**
 * Private
 */

Lumen::RHIOpenGL::FOpenGLSwapchain::FSlot::FSlot ( TSharedRef<RHI::IRHITexture> InImage,
                                                   TSharedRef<FOpenGLSemaphore> InAcquireSemaphore,
                                                   TSharedRef<FOpenGLSemaphore> InPresentSemaphore ) noexcept
    : Image( static_cast<TSharedRef<RHI::IRHITexture> &&>( InImage ) ),
      AcquireSemaphore( static_cast<TSharedRef<FOpenGLSemaphore> &&>( InAcquireSemaphore ) ),
      PresentSemaphore( static_cast<TSharedRef<FOpenGLSemaphore> &&>( InPresentSemaphore ) )
{
    /* */
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLSwapchain::RebuildImages () noexcept
{
    TVector<FSlot> Replacement;
    Replacement.reserve( static_cast<USize>( Desc.ImageCount ) );
    for ( UInt32 Index = 0U; Index < Desc.ImageCount; ++Index )
    {
        LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Swapchain::RebuildImages" ) );
        RHI::FRHITextureDesc TextureDesc;
        TextureDesc.Extent       = Desc.Extent;
        TextureDesc.Format       = Desc.Format;
        TextureDesc.MipCount     = 1U;
        TextureDesc.LayerCount   = 1U;
        TextureDesc.SampleCount  = RHI::ERHISampleCount::Count1;
        TextureDesc.Usage        = Desc.ImageUsage;
        TextureDesc.InitialState = RHI::ERHIResourceState::Present;
        const RHI::FRHIObjectId ImageId{ Identity.GetId().Value + 1ULL + static_cast<UInt64>( Index ) };
        LUMEN_EXPECT( Texture, FOpenGLTexture::Create( Context, TextureDesc, ImageId ) );

        RHI::FRHISemaphoreDesc BinaryDesc;
        BinaryDesc.Type = RHI::ERHISemaphoreType::Binary;
        const RHI::FRHIObjectId AcquireId{ ImageId.Value + 1000ULL + static_cast<UInt64>( Desc.ImageCount ) };
        LUMEN_EXPECT( Acquire, FOpenGLSemaphore::Create( Context, BinaryDesc, AcquireId ) );
        const RHI::FRHIObjectId ReleaseId{ AcquireId.Value + 1000ULL };
        LUMEN_EXPECT( Release, FOpenGLSemaphore::Create( Context, BinaryDesc, ReleaseId ) );

        Replacement.emplace_back( static_cast<TSharedRef<RHI::IRHITexture>>( Texture ), Acquire, Release );
    }
    Slots  = static_cast<TVector<FSlot> &&>( Replacement );
    Cursor = 0U;
    return {};
}

Lumen::RHI::FRHIExpected
Lumen::RHIOpenGL::FOpenGLSwapchain::PresentToBridge ( RHI::IRHITexture &InImage ) noexcept
{
    if ( Desc.Surface.Type != RHI::ERHISurfaceType::Window or not Desc.Surface.Bridge or not NativeSurface )
    {
        return {};
    }

    LUMEN_TRY_EXPECT_ERROR( Desc.Surface.Bridge->MakeCurrent( *NativeSurface ) );
    const TOptional<FOpenGLTexture *> NativeImage = RHI::TryRHIBackendCast<FOpenGLTexture>( InImage );
    if ( not NativeImage )
    {
        return TUnexpected( RHI::MakeRHIInvalidPresentation( "The acquired image is not an OpenGL texture." ) );
    }

    const FGLDispatchTable &Dispatch = Context->GetDispatch();
    const GLuint Texture             = ( *NativeImage )->GetTexture();
    const GLint Width                = static_cast<GLint>( Desc.Extent.Width );
    const GLint Height               = static_cast<GLint>( Desc.Extent.Height );
    RHI::FRHIExpected Result{};
    LUMEN_TRY_EXPECT_ERROR( Context->GetExecutor()->ExecuteOnGLThread( [this, &Dispatch, Texture, Width, Height, &Result] () noexcept
    {
        ( void )Dispatch.GetError();
        if ( PresentationFramebuffer == 0U )
        {
            Dispatch.CreateFramebuffers( 1, &PresentationFramebuffer );
        }
        if ( PresentationFramebuffer == 0U )
        {
            Result = TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Presentation,
                                                             "Unable to create the OpenGL presentation framebuffer." ) );
            return;
        }

        Dispatch.NamedFramebufferTexture( PresentationFramebuffer, GlColorAttachment0, Texture, 0 );
        Dispatch.NamedFramebufferReadBuffer( PresentationFramebuffer, GlColorAttachment0 );
        if ( Dispatch.CheckNamedFramebufferStatus( PresentationFramebuffer, GlReadFramebuffer ) != GlFramebufferComplete )
        {
            Result = TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Presentation,
                                                             "The OpenGL presentation framebuffer is incomplete." ) );
            return;
        }

        Dispatch.Disable( GlFramebufferSrgb );
        Dispatch.BindFramebuffer( GlReadFramebuffer, PresentationFramebuffer );
        Dispatch.BindFramebuffer( GlDrawFramebuffer, 0U );
        Dispatch.BlitFramebuffer( 0, 0, Width, Height, 0, 0, Width, Height, GlColorBufferBit, GlNearest );
        Dispatch.BindFramebuffer( GlReadFramebuffer, 0U );
        Dispatch.BindFramebuffer( GlDrawFramebuffer, 0U );
        const GLenum Error = Dispatch.GetError();
        if ( Error != GlNoError )
        {
            Result = TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Presentation,
                                                             "OpenGL failed to blit the acquired image to the native window." ) );
        }
    } ) );
    if ( not Result )
    {
        ( void )Context->GetExecutor()->ExecuteOnGLThread( [this] () noexcept
        {
            ( void )Context->GetExecutor()->GetNativeContext().MakeCurrent();
        } );
        return Result;
    }

    const Int32 SwapInterval = Desc.PresentMode == RHI::ERHIPresentMode::Immediate ? 0 : 1;
    return Desc.Surface.Bridge->Present( *NativeSurface, SwapInterval );
}
