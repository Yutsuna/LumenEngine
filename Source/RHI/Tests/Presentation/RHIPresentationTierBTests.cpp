/**
 * @file RHIPresentationTierBTests.cpp
 * @brief Tests swapchain stale tokens, exhaustion, and suboptimal states.
 */

#include <Device/NullDevice.hpp>
#include <Device/RHIBackendCast.hpp>
#include <Presentation/NullPresentation.hpp>
#include <Queue/IRHIQueue.hpp>
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
        Desc.Surface.InitialExtent = { 256U, 256U, 1U };
        Desc.Extent                = Desc.Surface.InitialExtent;
        Desc.ImageCount            = 2U;
        return Desc;
    }

    LUMEN_TEST( RHIPresentationTierB, RejectsExhaustionStaleTokensAndWrongQueues )
    {
        Tests::FNullDevice Device;
        const TSharedRef<IRHISwapchain> Swapchain = *Device.CreateSwapchain( MakeDesc() );
        const TSharedRef<IRHIQueue> Graphics      = *Device.GetQueue( ERHIQueueType::Graphics, 0U );
        const TSharedRef<IRHIQueue> Copy          = *Device.GetQueue( ERHIQueueType::Copy, 0U );
        const FRHIAcquiredImage First             = *Swapchain->AcquireNextImage();
        const FRHIAcquiredImage Second            = *Swapchain->AcquireNextImage();
        LUMEN_CHECK_HAS_ERROR( Swapchain->AcquireNextImage() );
        LUMEN_CHECK_HAS_ERROR( Swapchain->Present( *Copy, First, nullptr ) );
        LUMEN_CHECK_TRUE( Swapchain->Present( *Graphics, First, nullptr ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( Swapchain->Present( *Graphics, First, nullptr ) );
        LUMEN_CHECK_TRUE( Swapchain->Present( *Graphics, Second, nullptr ).HasValue() );
    }

    LUMEN_TEST( RHIPresentationTierB, ReportsSuboptimalAndOutOfDateExplicitly )
    {
        Tests::FNullDevice Device;
        const TSharedRef<IRHISwapchain> Created = *Device.CreateSwapchain( MakeDesc() );
        const TSharedPtr<Tests::FNullSwapchain> NullSwapchain =
            *TryRHIBackendCast<Tests::FNullSwapchain>( StaticCastSharedRef<IRHIObject>( Created ) );
        NullSwapchain->MarkSuboptimal();
        const FRHIAcquiredImage Acquired = *Created->AcquireNextImage();
        LUMEN_CHECK_TRUE( Acquired.bSuboptimal );
        const TSharedRef<IRHIQueue> Queue = *Device.GetQueue( ERHIQueueType::Graphics, 0U );
        LUMEN_CHECK_TRUE( Created->Present( *Queue, Acquired, nullptr ).HasValue() );
        NullSwapchain->MarkOutOfDate();
        const TExpected<FRHIAcquiredImage, FRHIError> Result = Created->AcquireNextImage();
        LUMEN_CHECK_TRUE( Result.Error().Code == ERHIErrorCode::OutOfDate );
        LUMEN_CHECK_TRUE( Created->Resize( { 512U, 512U, 1U } ).HasValue() );
        LUMEN_CHECK_TRUE( Created->AcquireNextImage().HasValue() );
    }

} // namespace
} // namespace Lumen
