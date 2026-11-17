/**
 * @file RHIValidationSwapchainTests.cpp
 * @brief Tests acquired-image tokens, presentation, and resize validation.
 */
#include "Validation/RHIValidationReporter.hpp"
#include "Validation/RHIValidationTestUtils.hpp"
#include <Presentation/NullPresentation.hpp>
#include <Queue/IRHIQueue.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIValidationSwapchain, RejectsDoublePresentAndResizeWhileAcquired )
    {
        Tests::FRecordingDebugSink Sink;
        auto Device    = Tests::Validation::MakeDevice( Sink );
        auto Swapchain = *Device->CreateSwapchain( Tests::Validation::MakeSwapchainDesc() );
        auto Queue     = *Device->GetQueue( ERHIQueueType::Graphics, 0U );
        auto Acquired  = *Swapchain->AcquireNextImage();
        LUMEN_CHECK_TRUE( Acquired.Image.IsValid() );
        LUMEN_CHECK_EQ( Swapchain->Resize( { 800U, 600U, 1U } ).Error().Code, ERHIErrorCode::ValidationFailed );
        LUMEN_CHECK_TRUE( Swapchain->Present( *Queue, Acquired, nullptr ).HasValue() );
        LUMEN_CHECK_EQ( Swapchain->Present( *Queue, Acquired, nullptr ).Error().Code, ERHIErrorCode::ValidationFailed );
        auto NullSwapchain = TryUnwrapRHIBackend<Tests::FNullSwapchain>( Swapchain.Get() );
        LUMEN_CHECK_TRUE( NullSwapchain != nullptr );
        LUMEN_CHECK_EQ( NullSwapchain->GetPresentCount(), 1ULL );
        LUMEN_CHECK_TRUE( Swapchain->Resize( { 800U, 600U, 1U } ).HasValue() );
        auto Image = *Swapchain->GetImage( 0U );
        LUMEN_CHECK_EQ( Image->GetDesc().Extent.Width, 800U );
        LUMEN_CHECK_EQ( Swapchain->GetImage( 99U ).Error().Code, ERHIErrorCode::ValidationFailed );
    }

    LUMEN_TEST( RHIValidationSwapchain, RejectsAcquireWhenEveryImageIsOutstanding )
    {
        Tests::FRecordingDebugSink Sink;
        auto Device    = Tests::Validation::MakeDevice( Sink );
        auto Swapchain = *Device->CreateSwapchain( Tests::Validation::MakeSwapchainDesc() );
        TVector<FRHIAcquiredImage> Acquired;
        for ( UInt32 Index = 0U; Index < Swapchain->GetImageCount(); ++Index )
        {
            auto Image = Swapchain->AcquireNextImage();
            LUMEN_CHECK_TRUE( Image.HasValue() );
            Acquired.push_back( *Image );
        }
        LUMEN_CHECK_EQ( Swapchain->AcquireNextImage().Error().Code, ERHIErrorCode::ValidationFailed );
        LUMEN_CHECK_TRUE( Sink.ContainsCode( static_cast<UInt32>( ERHIValidationMessageCode::InvalidPresentation ) ) );
    }
} // namespace
} // namespace Lumen
