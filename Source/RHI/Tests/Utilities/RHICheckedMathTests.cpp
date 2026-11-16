/**
 * @file RHICheckedMathTests.cpp
 * @brief Tests overflow-safe arithmetic and casts.
 */

#include <Tests/UnitTest.hpp>
#include <Utilities/RHICheckedMath.hpp>
#include <limits>

namespace Lumen
{

LUMEN_TEST( RHICheckedMath, DetectsOverflowAndInvalidConversions )
{
    LUMEN_CHECK_EQ( RHICheckedAdd( 2U, 3U ).ValueOr( 0U ), 5U );
    LUMEN_CHECK_FALSE( RHICheckedAdd( std::numeric_limits<UInt32>::max(), 1U ).HasValue() );
    LUMEN_CHECK_EQ( RHICheckedMultiply( 6U, 7U ).ValueOr( 0U ), 42U );
    LUMEN_CHECK_EQ( RHICheckedDivideCeil( 17U, 4U ).ValueOr( 0U ), 5U );
    LUMEN_CHECK_FALSE( RHICheckedDivideCeil( 1U, 0U ).HasValue() );
    LUMEN_CHECK_FALSE( RHICheckedCast<UInt32>( -1 ).HasValue() );
    LUMEN_CHECK_EQ( RHICheckedCast<UInt8>( 255U ).ValueOr( 0U ), 255U );
    LUMEN_CHECK_FALSE( RHICheckedCast<UInt8>( 256U ).HasValue() );
}

} // namespace Lumen
