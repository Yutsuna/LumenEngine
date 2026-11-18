/**
 * @file RHISubresourceRangeTests.cpp
 * @brief Tests subresource containment and intersection.
 */

#include <RHI/Resources/RHISubresourceRange.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHISubresourceRange, ComputesIntersectionAndBounds )
    {
        const FRHISubresourceRange First{ .BaseMip = 0U, .MipCount = 4U, .BaseLayer = 0U, .LayerCount = 4U };
        const FRHISubresourceRange Second{ .BaseMip = 2U, .MipCount = 4U, .BaseLayer = 1U, .LayerCount = 2U };
        const TOptional<FRHISubresourceRange> Intersection = First.Intersect( Second );

        LUMEN_CHECK_TRUE( Intersection.HasValue() );
        LUMEN_CHECK_EQ( Intersection->BaseMip, 2U );
        LUMEN_CHECK_EQ( Intersection->MipCount, 2U );
        LUMEN_CHECK_TRUE( First.IsContainedIn( 4U, 4U ) );
        LUMEN_CHECK_FALSE( Second.IsContainedIn( 4U, 4U ) );
    }

} // namespace RHI

} // namespace Lumen
