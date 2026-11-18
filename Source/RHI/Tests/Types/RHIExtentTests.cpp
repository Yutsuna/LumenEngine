/**
 * @file RHIExtentTests.cpp
 * @brief Tests extents, offsets, rectangles, and viewports.
 */

#include <RHI/Types/RHIExtent.hpp>
#include <RHI/Types/RHIViewport.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIExtent, ValidatesBounds )
    {
        constexpr FRHIExtent3D Extent{ 4U, 8U, 2U };
        LUMEN_CHECK_TRUE( Extent.IsValid() );
        LUMEN_CHECK_TRUE( Extent.Contains( 3U, 7U, 1U ) );
        LUMEN_CHECK_FALSE( Extent.Contains( 4U, 0U, 0U ) );
        LUMEN_CHECK_TRUE( FRHIOffset3D{}.IsNonNegative() );
        LUMEN_CHECK_FALSE( ( FRHIOffset3D{ -1, 0, 0 }.IsNonNegative() ) );
        LUMEN_CHECK_TRUE( ( FRHIRect2D{ 0, 0, 4U, 4U }.IsValid() ) );
        LUMEN_CHECK_TRUE( ( FRHIViewport{ 0.0F, 0.0F, 4.0F, 4.0F }.IsValid() ) );
    }

    LUMEN_TEST( RHIExtent, ComputesMaxMipLevelCount )
    {
        LUMEN_CHECK_EQ( ( FRHIExtent3D{ 1U, 1U, 1U }.GetMaxMipLevelCount() ), 1U );
        LUMEN_CHECK_EQ( ( FRHIExtent3D{ 16U, 8U, 1U }.GetMaxMipLevelCount() ), 5U );
        LUMEN_CHECK_EQ( ( FRHIExtent3D{ 100U, 1U, 1U }.GetMaxMipLevelCount() ), 7U );
        LUMEN_CHECK_EQ( ( FRHIExtent3D{ 0xFFFFFFFFU, 1U, 1U }.GetMaxMipLevelCount() ), 32U );
    }

} // namespace RHI

} // namespace Lumen
