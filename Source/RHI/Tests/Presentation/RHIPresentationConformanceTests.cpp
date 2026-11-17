/**
 * @file RHIPresentationConformanceTests.cpp
 * @brief Runs portable swapchain creation, acquisition, presentation, and resize.
 */

#include <Device/IRHIDevice.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>
#include <Presentation/IRHISwapchain.hpp>
#include <Queue/IRHIQueue.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIPresentationConformance, PresentsAndRecreatesAnImageRing )
    {
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        if ( not( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::Presentation ) )
        {
            return;
        }
        TExpected<TSharedRef<IRHISwapchain>, FRHIError> Swapchain =
            Tests::GetRHIConformanceDeviceFactory().CreateSwapchain( **Device );
        LUMEN_CHECK_TRUE( Swapchain.HasValue() );
        const TSharedRef<IRHIQueue> Queue                = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        TExpected<FRHIAcquiredImage, FRHIError> Acquired = ( *Swapchain )->AcquireNextImage();
        LUMEN_CHECK_TRUE( Acquired.HasValue() );
        LUMEN_CHECK_TRUE( Acquired->Image.Get() != nullptr );
        LUMEN_CHECK_TRUE( ( *Swapchain )->Present( *Queue, *Acquired, nullptr ).HasValue() );
        LUMEN_CHECK_TRUE( ( *Swapchain )->Resize( { 640U, 400U, 1U } ).HasValue() );
        LUMEN_CHECK_EQ( ( *Swapchain )->GetDesc().Extent.Width, 640U );

        TExpected<TSharedRef<IRHISwapchain>, FRHIError> Replacement =
            Tests::GetRHIConformanceDeviceFactory().CreateSwapchain( **Device, *Swapchain );
        LUMEN_CHECK_TRUE( Replacement.HasValue() );
        LUMEN_CHECK_TRUE( ( *Replacement )->GetDesc().OldSwapchain.Get() == ( *Swapchain ).Get() );
    }

    LUMEN_TEST( RHIPresentationConformance, RejectsExhaustionStaleTokensAndInvalidResize )
    {
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        if ( not( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::Presentation ) )
        {
            return;
        }
        TExpected<TSharedRef<IRHISwapchain>, FRHIError> Swapchain =
            Tests::GetRHIConformanceDeviceFactory().CreateSwapchain( **Device );
        LUMEN_CHECK_TRUE( Swapchain.HasValue() );
        const TExpected<TSharedRef<IRHITexture>, FRHIError> InvalidImage =
            ( *Swapchain )->GetImage( ( *Swapchain )->GetImageCount() );
        LUMEN_CHECK_HAS_ERROR( InvalidImage );
        LUMEN_CHECK_EQ( InvalidImage.Error().Code, ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( InvalidImage.Error().Condition, ERHIErrorCondition::Presentation );

        TVector<FRHIAcquiredImage> AcquiredImages;
        for ( UInt32 Index = 0U; Index < ( *Swapchain )->GetImageCount(); ++Index )
        {
            TExpected<FRHIAcquiredImage, FRHIError> Acquired = ( *Swapchain )->AcquireNextImage();
            LUMEN_CHECK_TRUE( Acquired.HasValue() );
            AcquiredImages.push_back( *Acquired );
        }
        const TExpected<FRHIAcquiredImage, FRHIError> Exhausted = ( *Swapchain )->AcquireNextImage();
        LUMEN_CHECK_HAS_ERROR( Exhausted );
        LUMEN_CHECK_EQ( Exhausted.Error().Code, ERHIErrorCode::InvalidState );
        const TExpected<void, FRHIError> InvalidResize = ( *Swapchain )->Resize( { 640U, 400U, 1U } );
        LUMEN_CHECK_HAS_ERROR( InvalidResize );
        LUMEN_CHECK_EQ( InvalidResize.Error().Condition, ERHIErrorCondition::Presentation );

        const TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        LUMEN_CHECK_TRUE( ( *Swapchain )->Present( *Queue, AcquiredImages[0], nullptr ).HasValue() );
        const TExpected<void, FRHIError> Stale = ( *Swapchain )->Present( *Queue, AcquiredImages[0], nullptr );
        LUMEN_CHECK_HAS_ERROR( Stale );
        LUMEN_CHECK_EQ( Stale.Error().Code, ERHIErrorCode::InvalidState );
        for ( USize Index = 1U; Index < AcquiredImages.size(); ++Index )
        {
            LUMEN_CHECK_TRUE( ( *Swapchain )->Present( *Queue, AcquiredImages[Index], nullptr ).HasValue() );
        }
        LUMEN_CHECK_TRUE( ( *Swapchain )->Resize( { 640U, 400U, 1U } ).HasValue() );
    }

    LUMEN_TEST( RHIPresentationConformance, ReportsOutOfDateAndDeviceLossExplicitly )
    {
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        if ( not( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::Presentation ) )
        {
            return;
        }
        TExpected<TSharedRef<IRHISwapchain>, FRHIError> Swapchain =
            Tests::GetRHIConformanceDeviceFactory().CreateSwapchain( **Device );
        LUMEN_CHECK_TRUE( Swapchain.HasValue() );
        if ( Tests::GetRHIConformanceDeviceFactory().InjectSwapchainOutOfDate( **Swapchain ) )
        {
            const TExpected<FRHIAcquiredImage, FRHIError> OutOfDate = ( *Swapchain )->AcquireNextImage();
            LUMEN_CHECK_HAS_ERROR( OutOfDate );
            LUMEN_CHECK_EQ( OutOfDate.Error().Code, ERHIErrorCode::OutOfDate );
            LUMEN_CHECK_EQ( OutOfDate.Error().Condition, ERHIErrorCondition::Presentation );
            LUMEN_CHECK_TRUE( ( *Swapchain )->Resize( { 640U, 400U, 1U } ).HasValue() );
            LUMEN_CHECK_TRUE( ( *Swapchain )->AcquireNextImage().HasValue() );
        }

        if ( not Tests::GetRHIConformanceDeviceFactory().InjectDeviceLost( **Device ) )
        {
            return;
        }
        const TExpected<TSharedRef<IRHISwapchain>, FRHIError> Replacement =
            Tests::GetRHIConformanceDeviceFactory().CreateSwapchain( **Device );
        LUMEN_CHECK_HAS_ERROR( Replacement );
        LUMEN_CHECK_EQ( Replacement.Error().Code, ERHIErrorCode::DeviceLost );
        const TExpected<FRHIAcquiredImage, FRHIError> Acquired = ( *Swapchain )->AcquireNextImage();
        LUMEN_CHECK_HAS_ERROR( Acquired );
        LUMEN_CHECK_EQ( Acquired.Error().Code, ERHIErrorCode::DeviceLost );
    }

} // namespace
} // namespace Lumen
