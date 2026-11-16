/**
 * @file RHIExtentTests.cpp
 * @brief Tests extents, offsets, rectangles, and viewports.
 */

#include <Tests/UnitTest.hpp>
#include <Types/RHIExtent.hpp>
#include <Types/RHIViewport.hpp>

namespace Lumen
{

LUMEN_TEST( RHIExtent, ValidatesBounds )
{
    constexpr FRHIExtent3D Extent{ .Width = 4U, .Height = 8U, .Depth = 2U };
    LUMEN_CHECK_TRUE( Extent.IsValid() );
    LUMEN_CHECK_TRUE( Extent.Contains( 3U, 7U, 1U ) );
    LUMEN_CHECK_FALSE( Extent.Contains( 4U, 0U, 0U ) );
    LUMEN_CHECK_TRUE( FRHIOffset3D{}.IsNonNegative() );
    LUMEN_CHECK_FALSE( ( FRHIOffset3D{ -1, 0, 0 }.IsNonNegative() ) );
    LUMEN_CHECK_TRUE( ( FRHIRect2D{ 0, 0, 4U, 4U }.IsValid() ) );
    LUMEN_CHECK_TRUE( ( FRHIViewport{ 0.0F, 0.0F, 4.0F, 4.0F }.IsValid() ) );
}

} // namespace Lumen
