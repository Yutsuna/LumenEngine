/**
 * @file RHISwapchainCapabilitiesTests.cpp
 * @brief Tests swapchain capability negotiation and extent clamping.
 */

#include <Presentation/NullPresentation.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHISwapchainCapabilities, SelectsOnlyAdvertisedCombinations )
        {
            const FRHISwapchainCapabilities Capabilities = Tests::MakeNullSwapchainCapabilities();
            LUMEN_CHECK_TRUE( Capabilities.Validate().HasValue() );
            LUMEN_CHECK_TRUE(
                Capabilities.Supports( ERHIFormat::RGBA8Unorm, ERHIColorSpace::SrgbNonLinear, ERHIPresentMode::Mailbox ) );
            LUMEN_CHECK_FALSE(
                Capabilities.Supports( ERHIFormat::BC7RGBAUnorm, ERHIColorSpace::SrgbNonLinear, ERHIPresentMode::Fifo ) );
            const FRHIExtent3D Clamped = Capabilities.ClampExtent( { 0U, 50000U, 1U } );
            LUMEN_CHECK_EQ( Clamped.Width, 1U );
            LUMEN_CHECK_EQ( Clamped.Height, 16384U );
        }

        LUMEN_TEST( RHISwapchainCapabilities, RejectsIncoherentRanges )
        {
            FRHISwapchainCapabilities Capabilities = Tests::MakeNullSwapchainCapabilities();
            Capabilities.Formats.clear();
            LUMEN_CHECK_HAS_ERROR( Capabilities.Validate() );
            Capabilities               = Tests::MakeNullSwapchainCapabilities();
            Capabilities.MaxImageCount = 1U;
            LUMEN_CHECK_HAS_ERROR( Capabilities.Validate() );
            Capabilities                 = Tests::MakeNullSwapchainCapabilities();
            Capabilities.MaxExtent.Width = 0U;
            LUMEN_CHECK_HAS_ERROR( Capabilities.Validate() );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
