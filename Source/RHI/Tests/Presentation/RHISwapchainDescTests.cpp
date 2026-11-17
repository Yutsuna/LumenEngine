/**
 * @file RHISwapchainDescTests.cpp
 * @brief Tests swapchain descriptor validation against surface capabilities.
 */

#include <Presentation/NullPresentation.hpp>
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
        Desc.Surface.InitialExtent = { 1280U, 720U, 1U };
        Desc.Extent                = Desc.Surface.InitialExtent;
        return Desc;
    }

    LUMEN_TEST( RHISwapchainDesc, AcceptsSupportedConfiguration )
    {
        const FRHISwapchainDesc Desc = MakeDesc();
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        LUMEN_CHECK_TRUE( Desc.Validate( Tests::MakeNullSwapchainCapabilities() ).HasValue() );
    }

    LUMEN_TEST( RHISwapchainDesc, RejectsInvalidAndUnsupportedConfigurations )
    {
        FRHISwapchainDesc Desc = MakeDesc();
        Desc.ImageCount        = 1U;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc            = MakeDesc();
        Desc.ImageUsage = ERHIResourceUsage::ShaderResource;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc        = MakeDesc();
        Desc.Format = ERHIFormat::BC7RGBAUnorm;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate( Tests::MakeNullSwapchainCapabilities() ) );
        Desc        = MakeDesc();
        Desc.Extent = { 20000U, 720U, 1U };
        LUMEN_CHECK_HAS_ERROR( Desc.Validate( Tests::MakeNullSwapchainCapabilities() ) );
    }

} // namespace
} // namespace Lumen
