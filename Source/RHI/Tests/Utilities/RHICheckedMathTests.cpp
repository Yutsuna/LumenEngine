/**
 * @file RHICheckedMathTests.cpp
 * @brief Tests overflow-safe arithmetic and casts.
 */

#include <RHI/Utilities/RHICheckedMath.hpp>
#include <Tests/UnitTest.hpp>
#include <limits>

namespace Lumen
{

namespace RHI
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

    LUMEN_TEST( RHICheckedMath, SignedToSignedWideningPreservesValue )
    {
        LUMEN_CHECK_EQ( RHICheckedCast<Int64>( Int32( 5 ) ).ValueOr( 0LL ), 5LL );
        LUMEN_CHECK_EQ( RHICheckedCast<Int64>( Int32( -5 ) ).ValueOr( 0LL ), -5LL );
    }

    LUMEN_TEST( RHICheckedMath, SignedToSignedNarrowingRejectsOutOfRange )
    {
        LUMEN_CHECK_FALSE( RHICheckedCast<Int32>( std::numeric_limits<Int64>::max() ).HasValue() );
        LUMEN_CHECK_FALSE( RHICheckedCast<Int16>( Int32( 40000 ) ).HasValue() );
    }

    LUMEN_TEST( RHICheckedMath, UnsignedCastsRespectRange )
    {
        LUMEN_CHECK_EQ( RHICheckedCast<UInt64>( UInt32( 7 ) ).ValueOr( 0ULL ), 7ULL );
        LUMEN_CHECK_FALSE( RHICheckedCast<UInt8>( UInt32( 300 ) ).HasValue() );
    }

} // namespace RHI

} // namespace Lumen
