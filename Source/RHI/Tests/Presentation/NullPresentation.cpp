/**
 * @file NullPresentation.cpp
 * @brief Implements deterministic Null swapchain acquisition and presentation.
 */

#include <Device/IRHIDevice.hpp>
#include <HAL/LockGuard.hpp>
#include <Presentation/NullPresentation.hpp>
#include <Queue/IRHIQueue.hpp>
#include <Resources/IRHITexture.hpp>
#include <Synchronization/IRHISemaphore.hpp>
#include <Templates/Hash.hpp>

Lumen::FRHISwapchainCapabilities Lumen::Tests::MakeNullSwapchainCapabilities () noexcept
{
    FRHISwapchainCapabilities Capabilities;
    Capabilities.Formats      = { ERHIFormat::RGBA8Unorm, ERHIFormat::BGRA8Unorm, ERHIFormat::RGBA16Float };
    Capabilities.ColorSpaces  = { ERHIColorSpace::SrgbNonLinear, ERHIColorSpace::ExtendedSrgbLinear };
    Capabilities.PresentModes = { ERHIPresentMode::Fifo, ERHIPresentMode::Immediate, ERHIPresentMode::Mailbox };
    Capabilities.MinExtent    = { 1U, 1U, 1U };
    Capabilities.MaxExtent    = { 16384U, 16384U, 1U };
    return Capabilities;
}

Lumen::Tests::FNullSwapchain::FNullSwapchain ( const FRHISwapchainDesc &InDesc,
                                               const FRHISwapchainCapabilities &InCapabilities,
                                               IRHIDevice &InDevice,
                                               TSharedPtr<FNullFaultPlan> InFaultPlan,
                                               const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullSwapchain>() ), Desc( InDesc ), Capabilities( InCapabilities ), Device( &InDevice ),
      FaultPlan( std::move( InFaultPlan ) )
{
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullSwapchain )

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullSwapchain::Initialize () noexcept
{
    return RebuildImages();
}

const Lumen::FRHISwapchainDesc &Lumen::Tests::FNullSwapchain::GetDesc () const noexcept
{
    return Desc;
}

const Lumen::FRHISwapchainCapabilities &Lumen::Tests::FNullSwapchain::GetCapabilities () const noexcept
{
    return Capabilities;
}

Lumen::UInt32 Lumen::Tests::FNullSwapchain::GetImageCount () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return static_cast<UInt32>( Images.size() );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHITexture>, Lumen::FRHIError>
Lumen::Tests::FNullSwapchain::GetImage ( const UInt32 InIndex ) const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( InIndex >= Images.size() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPresentation( "Swapchain image index is out of range" ) );
    }
    return Images[InIndex];
}

Lumen::TExpected<Lumen::FRHIAcquiredImage, Lumen::FRHIError> Lumen::Tests::FNullSwapchain::AcquireNextImage () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( Device->GetStatus() != ERHIDeviceStatus::Ready )
    {
        const ERHIErrorCode Code =
            Device->GetStatus() == ERHIDeviceStatus::Lost ? ERHIErrorCode::DeviceLost : ERHIErrorCode::DeviceClosed;
        return TUnexpected<FRHIError>( FRHIError( Code, ERHIErrorCondition::Device, "Swapchain device is unavailable" ) );
    }
    if ( FaultPlan )
    {
        if ( auto Error = FaultPlan->ShouldFail( "AcquireNextImage" ) )
        {
            return TUnexpected<FRHIError>( *Error );
        }
    }
    if ( bOutOfDate )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( OutOfDate, ERHIErrorCondition::Presentation, "Null swapchain is out of date" ) );
    }
    const UInt32 ImageCount = static_cast<UInt32>( Images.size() );
    if ( ImageCount == 0U )
    {
        return TUnexpected<FRHIError>(
            FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Presentation, "Swapchain has no initialized images" ) );
    }
    UInt32 Selected = ImageCount;
    for ( UInt32 Offset = 0U; Offset < ImageCount; ++Offset )
    {
        const UInt32 Candidate = ( Cursor + Offset ) % ImageCount;
        if ( not Acquired[Candidate] )
        {
            Selected = Candidate;
            break;
        }
    }
    if ( Selected == ImageCount )
    {
        return TUnexpected<FRHIError>(
            FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Presentation, "Every swapchain image is acquired" ) );
    }
    if ( PresentSemaphores[Selected]->GetValue() != 0ULL )
    {
        const auto Reclaimed = PresentSemaphores[Selected]->Wait( 1ULL, 0.0 );
        if ( not Reclaimed )
        {
            return TUnexpected<FRHIError>( Reclaimed.Error() );
        }
    }
    const auto Signaled = AcquireSemaphores[Selected]->Signal( 1ULL );
    if ( not Signaled )
    {
        return TUnexpected<FRHIError>( Signaled.Error() );
    }
    Acquired[Selected] = true;
    Cursor             = ( Selected + 1U ) % ImageCount;
    FRHIAcquiredImage Result;
    Result.ImageIndex       = Selected;
    Result.Image            = Images[Selected];
    Result.AcquireSemaphore = AcquireSemaphores[Selected];
    Result.PresentSemaphore = PresentSemaphores[Selected];
    Result.bSuboptimal      = bSuboptimal;
    bSuboptimal             = false;
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullSwapchain::Present ( IRHIQueue &InQueue,
                                                                                 const FRHIAcquiredImage &InImage,
                                                                                 IRHISemaphore *InRenderComplete ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( Device->GetStatus() != ERHIDeviceStatus::Ready )
    {
        const ERHIErrorCode Code =
            Device->GetStatus() == ERHIDeviceStatus::Lost ? ERHIErrorCode::DeviceLost : ERHIErrorCode::DeviceClosed;
        return TUnexpected<FRHIError>( FRHIError( Code, ERHIErrorCondition::Device, "Swapchain device is unavailable" ) );
    }
    if ( FaultPlan )
    {
        if ( auto Error = FaultPlan->ShouldFail( "Present" ) )
        {
            return TUnexpected<FRHIError>( *Error );
        }
    }
    if ( bOutOfDate )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( OutOfDate, ERHIErrorCondition::Presentation, "Null swapchain is out of date" ) );
    }
    if ( InQueue.GetType() != ERHIQueueType::Graphics or InImage.ImageIndex >= Images.size() or not InImage.Image or
         not InImage.AcquireSemaphore or not InImage.PresentSemaphore )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPresentation( "Invalid presentation queue or acquired image" ) );
    }
    const UInt32 Index = InImage.ImageIndex;
    if ( not Acquired[Index] or InImage.Image != Images[Index] or InImage.AcquireSemaphore != AcquireSemaphores[Index] or
         InImage.PresentSemaphore != PresentSemaphores[Index] )
    {
        return TUnexpected<FRHIError>( FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Presentation,
                                                  "Acquired image token is stale or already presented" ) );
    }
    if ( InRenderComplete != nullptr )
    {
        if ( InRenderComplete->GetDesc().Type != ERHISemaphoreType::Binary )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidPresentation( "Render-complete semaphore must be binary" ) );
        }
        const auto Waited = InRenderComplete->Wait( 1ULL, 0.0 );
        if ( not Waited )
        {
            return Waited;
        }
    }
    if ( AcquireSemaphores[Index]->GetValue() != 0ULL )
    {
        const auto Consumed = AcquireSemaphores[Index]->Wait( 1ULL, 0.0 );
        if ( not Consumed )
        {
            return Consumed;
        }
    }
    const auto Signaled = PresentSemaphores[Index]->Signal( 1ULL );
    if ( not Signaled )
    {
        return Signaled;
    }
    Acquired[Index] = false;
    ++PresentCount;
    return {};
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullSwapchain::Resize ( const FRHIExtent3D &InExtent ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( Device->GetStatus() != ERHIDeviceStatus::Ready )
    {
        const ERHIErrorCode Code =
            Device->GetStatus() == ERHIDeviceStatus::Lost ? ERHIErrorCode::DeviceLost : ERHIErrorCode::DeviceClosed;
        return TUnexpected<FRHIError>( FRHIError( Code, ERHIErrorCondition::Device, "Swapchain device is unavailable" ) );
    }
    if ( FaultPlan )
    {
        if ( auto Error = FaultPlan->ShouldFail( "Resize" ) )
        {
            return TUnexpected<FRHIError>( *Error );
        }
    }
    for ( const Bool bAcquired : Acquired )
    {
        if ( bAcquired )
        {
            return TUnexpected<FRHIError>( FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Presentation,
                                                      "Cannot resize while an image is acquired" ) );
        }
    }
    FRHISwapchainDesc NewDesc     = Desc;
    NewDesc.Extent                = InExtent;
    NewDesc.Surface.InitialExtent = InExtent;
    const auto Valid              = NewDesc.Validate( Capabilities );
    if ( not Valid )
    {
        return Valid;
    }
    const FRHISwapchainDesc PreviousDesc = Desc;
    Desc                                 = NewDesc;
    const auto Rebuilt                   = RebuildImages();
    if ( not Rebuilt )
    {
        Desc = PreviousDesc;
    }
    else
    {
        bOutOfDate = false;
        Cursor     = 0U;
    }
    return Rebuilt;
}

void Lumen::Tests::FNullSwapchain::MarkOutOfDate () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    bOutOfDate = true;
}

void Lumen::Tests::FNullSwapchain::MarkSuboptimal () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    bSuboptimal = true;
}

Lumen::UInt64 Lumen::Tests::FNullSwapchain::GetPresentCount () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return PresentCount;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullSwapchain::RebuildImages () noexcept
{
    TVector<TSharedRef<IRHITexture>> NewImages;
    TVector<TSharedRef<IRHISemaphore>> NewAcquireSemaphores;
    TVector<TSharedRef<IRHISemaphore>> NewPresentSemaphores;
    for ( UInt32 Index = 0U; Index < Desc.ImageCount; ++Index )
    {
        FRHITextureDesc TextureDesc;
        TextureDesc.Extent       = Desc.Extent;
        TextureDesc.Format       = Desc.Format;
        TextureDesc.Usage        = Desc.ImageUsage;
        TextureDesc.InitialState = ERHIResourceState::Present;
        auto Texture             = Device->CreateTexture( TextureDesc );
        if ( not Texture )
        {
            return TUnexpected<FRHIError>( Texture.Error() );
        }
        FRHISemaphoreDesc SemaphoreDesc;
        SemaphoreDesc.Type = ERHISemaphoreType::Binary;
        auto Acquire       = Device->CreateSemaphore( SemaphoreDesc );
        if ( not Acquire )
        {
            return TUnexpected<FRHIError>( Acquire.Error() );
        }
        auto Present = Device->CreateSemaphore( SemaphoreDesc );
        if ( not Present )
        {
            return TUnexpected<FRHIError>( Present.Error() );
        }
        NewImages.push_back( *Texture );
        NewAcquireSemaphores.push_back( *Acquire );
        NewPresentSemaphores.push_back( *Present );
    }
    Images            = std::move( NewImages );
    AcquireSemaphores = std::move( NewAcquireSemaphores );
    PresentSemaphores = std::move( NewPresentSemaphores );
    Acquired.assign( Desc.ImageCount, false );
    return {};
}
