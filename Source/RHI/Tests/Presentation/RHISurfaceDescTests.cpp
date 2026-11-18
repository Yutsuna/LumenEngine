/**
 * @file RHISurfaceDescTests.cpp
 * @brief Tests backend-neutral surface payload validation.
 */

#include <Presentation/RHISurfaceDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHISurfaceDesc, AcceptsTypedCompletePayloads )
    {
        FRHISurfaceDesc Desc;
        Desc.Type          = ERHISurfaceType::Null;
        Desc.NativeWindow  = 1U;
        Desc.InitialExtent = { 1280U, 720U, 1U };
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );

        Desc.Type             = ERHISurfaceType::Window;
        Desc.NativeDisplay    = 2U;
        Desc.bRequiresDisplay = true;
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
    }

    LUMEN_TEST( RHISurfaceDesc, RejectsMissingHandlesAndExtents )
    {
        LUMEN_CHECK_HAS_ERROR( FRHISurfaceDesc{}.Validate() );
        FRHISurfaceDesc Desc;
        Desc.Type             = ERHISurfaceType::Window;
        Desc.NativeWindow     = 1U;
        Desc.InitialExtent    = { 1U, 1U, 1U };
        Desc.bRequiresDisplay = true;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.NativeDisplay = 2U;
        Desc.InitialExtent = { 0U, 1U, 1U };
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }

} // namespace
} // namespace Lumen
