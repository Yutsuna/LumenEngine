/**
 * @file RHIValidationSwapchain.cpp
 * @brief Implements validated swapchain image acquisition and presentation.
 */
#include "RHIValidationSwapchain.hpp"
#include <Queue/IRHIQueue.hpp>
#include <Synchronization/IRHISemaphore.hpp>
#include <Validation/RHIValidation.hpp>

Lumen::FRHIValidationSwapchain::FRHIValidationSwapchain ( TSharedRef<IRHISwapchain> InUnderlying,
                                                          TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::Swapchain );
    RebuildImages();
}

Lumen::FRHIValidationSwapchain::~FRHIValidationSwapchain () noexcept
{
    Images.clear();
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationSwapchain )

const Lumen::FRHISwapchainDesc &Lumen::FRHIValidationSwapchain::GetDesc () const noexcept
{
    return Underlying->GetDesc();
}

const Lumen::FRHISwapchainCapabilities &Lumen::FRHIValidationSwapchain::GetCapabilities () const noexcept
{
    return Underlying->GetCapabilities();
}

Lumen::UInt32 Lumen::FRHIValidationSwapchain::GetImageCount () const noexcept
{
    return static_cast<UInt32>( Images.size() );
}

void Lumen::FRHIValidationSwapchain::RebuildImages () noexcept
{
    TVector<TSharedRef<IRHITexture>> NewImages;
    const UInt32 Count = Underlying->GetImageCount();
    NewImages.reserve( Count );
    for ( UInt32 Index = 0U; Index < Count; ++Index )
    {
        auto Image = Underlying->GetImage( Index );
        if ( Image )
        {
            NewImages.push_back( MakeShared<FRHIValidationTexture>( *Image, State ) );
        }
    }
    Images = std::move( NewImages );
    Acquired.assign( Images.size(), false );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHITexture>, Lumen::FRHIError>
Lumen::FRHIValidationSwapchain::GetImage ( const UInt32 InIndex ) const noexcept
{
    if ( InIndex >= Images.size() )
    {
        return TUnexpected<FRHIError>( State->GetReporter()
                                           ->Reject( ERHIValidationMessageCode::InvalidPresentation,
                                                     ERHIErrorCondition::Presentation, "Swapchain image index is out of range",
                                                     this )
                                           .Error() );
    }
    return Images[InIndex];
}

Lumen::TExpected<Lumen::FRHIAcquiredImage, Lumen::FRHIError> Lumen::FRHIValidationSwapchain::AcquireNextImage () noexcept
{
    Bool bAllAcquired = not Acquired.empty();
    for ( const Bool bAcquired : Acquired )
    {
        bAllAcquired = bAllAcquired and bAcquired;
    }
    if ( bAllAcquired )
    {
        return TUnexpected<FRHIError>( State->GetReporter()
                                           ->Reject( ERHIValidationMessageCode::InvalidPresentation,
                                                     ERHIErrorCondition::Presentation, "Every swapchain image is acquired", this )
                                           .Error() );
    }
    auto Result = Underlying->AcquireNextImage();
    if ( not Result )
    {
        return Result;
    }
    if ( Result->ImageIndex >= Images.size() )
    {
        return TUnexpected<FRHIError>( State->GetReporter()
                                           ->Reject( ERHIValidationMessageCode::InvalidPresentation,
                                                     ERHIErrorCondition::Presentation, "Backend returned an invalid image index",
                                                     this )
                                           .Error() );
    }
    Acquired[Result->ImageIndex] = true;
    Result->Image                = Images[Result->ImageIndex];
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationSwapchain::Present ( IRHIQueue &InQueue,
                                                                                   const FRHIAcquiredImage &InImage,
                                                                                   IRHISemaphore *InRenderComplete ) noexcept
{
    auto QueueValid = State->ValidateObject( InQueue, ERHIValidationObjectKind::Queue );
    if ( not QueueValid )
    {
        return QueueValid;
    }
    if ( InImage.ImageIndex >= Images.size() or not InImage.Image or not Acquired[InImage.ImageIndex] or
         InImage.Image.Get() != Images[InImage.ImageIndex].Get() )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidPresentation, ERHIErrorCondition::Presentation,
                                             "Acquired image token is stale or already presented", this );
    }
    FRHIAcquiredImage Unwrapped = InImage;
    Unwrapped.Image             = UnwrapRHIShared( InImage.Image );
    auto Result                 = Underlying->Present( *UnwrapRHI( &InQueue ), Unwrapped, UnwrapRHI( InRenderComplete ) );
    if ( Result )
    {
        Acquired[InImage.ImageIndex] = false;
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationSwapchain::Resize ( const FRHIExtent3D &InExtent ) noexcept
{
    for ( const Bool bAcquired : Acquired )
    {
        if ( bAcquired )
        {
            return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidPresentation, ERHIErrorCondition::Presentation,
                                                 "Cannot resize while an image is acquired", this );
        }
    }
    auto Result = Underlying->Resize( InExtent );
    if ( Result )
    {
        RebuildImages();
    }
    return Result;
}
