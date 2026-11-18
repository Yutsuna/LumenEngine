/**
 * @file RHIAlignmentTests.cpp
 * @brief Tests checked alignment and range overlap.
 */

#include <RHI/Utilities/RHIAlignment.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIAlignment, AlignsAndRejectsInvalidInputs )
    {
        LUMEN_CHECK_TRUE( RHIIsPowerOfTwo( 64U ) );
        LUMEN_CHECK_FALSE( RHIIsPowerOfTwo( 3U ) );
        LUMEN_CHECK_EQ( RHIAlignUp( 17U, 16U ).ValueOr( 0U ), 32U );
        LUMEN_CHECK_EQ( RHIAlignDown( 31U, 16U ).ValueOr( 0U ), 16U );
        LUMEN_CHECK_FALSE( RHIAlignUp( 1U, 3U ).HasValue() );
        LUMEN_CHECK_TRUE( RHIRangesOverlap( 0ULL, 8ULL, 4ULL, 8ULL ) );
        LUMEN_CHECK_FALSE( RHIRangesOverlap( 0ULL, 4ULL, 4ULL, 8ULL ) );
    }

} // namespace RHI

} // namespace Lumen
