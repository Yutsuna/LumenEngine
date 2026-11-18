/**
 * @file RHISampleCountTests.cpp
 * @brief Tests sample-count conversions.
 */

#include <Tests/UnitTest.hpp>
#include <Types/RHISampleCount.hpp>

namespace Lumen
{

LUMEN_TEST( RHISampleCount, AcceptsOnlySupportedPowersOfTwo )
{
    LUMEN_CHECK_EQ( ToSampleCount( ERHISampleCount::Count1 ), 1U );
    LUMEN_CHECK_EQ( ToSampleCount( ERHISampleCount::Count64 ), 64U );
    LUMEN_CHECK_EQ( ToSampleCount( static_cast<ERHISampleCount>( 3U ) ), 0U );
}

} // namespace Lumen
