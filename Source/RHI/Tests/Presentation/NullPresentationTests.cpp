/**
 * @file NullPresentationTests.cpp
 * @brief Tests Null swapchain image cycling, presentation, and resizing.
 */

#include <Device/NullDevice.hpp>
#include <Device/RHIBackendCast.hpp>
#include <Presentation/NullPresentation.hpp>
#include <Queue/IRHIQueue.hpp>
#include <Resources/IRHITexture.hpp>
#include <Synchronization/IRHISemaphore.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    FRHISwapchainDesc MakeDesc () noexcept
    {
        FRHISwapchainDesc Desc;
        Desc.Surface.Type          = ERHISurfaceType::Null;
        Desc.Surface.NativeWindow  = 1U;
        Desc.Surface.InitialExtent = { 640U, 480U, 1U };
        Desc.Extent                = Desc.Surface.InitialExtent;
        return Desc;
    }

    LUMEN_TEST( NullPresentation, CyclesImagesAndPresentsWithSynchronization )
    {
        Tests::FNullDevice Device;
        TExpected<TSharedRef<IRHISwapchain>, FRHIError> Swapchain = Device.CreateSwapchain( MakeDesc() );
        LUMEN_CHECK_TRUE( Swapchain.HasValue() );
        LUMEN_CHECK_EQ( ( *Swapchain )->GetImageCount(), 3U );
        TSharedRef<IRHIQueue> Queue = *Device.GetQueue( ERHIQueueType::Graphics, 0U );
        for ( UInt32 Index = 0U; Index < 6U; ++Index )
        {
            TExpected<FRHIAcquiredImage, FRHIError> Acquired = ( *Swapchain )->AcquireNextImage();
            LUMEN_CHECK_TRUE( Acquired.HasValue() );
            LUMEN_CHECK_EQ( Acquired->ImageIndex, Index % 3U );
            LUMEN_CHECK_EQ( Acquired->AcquireSemaphore->GetValue(), 1ULL );
            LUMEN_CHECK_TRUE( ( *Swapchain )->Present( *Queue, *Acquired, nullptr ).HasValue() );
            LUMEN_CHECK_EQ( Acquired->PresentSemaphore->GetValue(), 1ULL );
        }
        TOptional<TSharedPtr<Tests::FNullSwapchain>> NullSwapchain =
            TryRHIBackendCast<Tests::FNullSwapchain>( StaticCastSharedRef<IRHIObject>( *Swapchain ) );
        LUMEN_CHECK_TRUE( NullSwapchain.HasValue() );
        LUMEN_CHECK_EQ( ( *NullSwapchain )->GetPresentCount(), 6ULL );
    }

    LUMEN_TEST( NullPresentation, ResizePreservesAcquiredImagesAndRebuildsAfterPresent )
    {
        Tests::FNullDevice Device;
        const TSharedRef<IRHISwapchain> Swapchain = *Device.CreateSwapchain( MakeDesc() );
        const TSharedRef<IRHIQueue> Queue         = *Device.GetQueue( ERHIQueueType::Graphics, 0U );
        const FRHIAcquiredImage Acquired          = *Swapchain->AcquireNextImage();
        LUMEN_CHECK_HAS_ERROR( Swapchain->Resize( { 800U, 600U, 1U } ) );
        LUMEN_CHECK_TRUE( Swapchain->Present( *Queue, Acquired, nullptr ).HasValue() );
        LUMEN_CHECK_TRUE( Swapchain->Resize( { 800U, 600U, 1U } ).HasValue() );
        const TSharedRef<IRHITexture> Image = *Swapchain->GetImage( 0U );
        LUMEN_CHECK_EQ( Image->GetDesc().Extent.Width, 800U );
        LUMEN_CHECK_EQ( Image->GetDesc().Extent.Height, 600U );
        LUMEN_CHECK_HAS_ERROR( Swapchain->GetImage( 99U ) );
    }

    LUMEN_TEST( NullPresentation, RequiresSignaledRenderCompleteSemaphore )
    {
        Tests::FNullDevice Device;
        const TSharedRef<IRHISwapchain> Swapchain = *Device.CreateSwapchain( MakeDesc() );
        const TSharedRef<IRHIQueue> Queue         = *Device.GetQueue( ERHIQueueType::Graphics, 0U );
        const FRHIAcquiredImage Acquired          = *Swapchain->AcquireNextImage();
        FRHISemaphoreDesc SemaphoreDesc;
        SemaphoreDesc.Type                       = ERHISemaphoreType::Binary;
        const TSharedRef<IRHISemaphore> Complete = *Device.CreateSemaphore( SemaphoreDesc );
        const TExpected<void, FRHIError> Timeout = Swapchain->Present( *Queue, Acquired, Complete.Get() );
        LUMEN_CHECK_TRUE( Timeout.Error().Code == ERHIErrorCode::Timeout );
        LUMEN_CHECK_TRUE( Complete->Signal( 1ULL ).HasValue() );
        LUMEN_CHECK_TRUE( Swapchain->Present( *Queue, Acquired, Complete.Get() ).HasValue() );
    }

} // namespace
} // namespace Lumen
