/**
 * @file RHIResourceLifetimeTests.cpp
 * @brief Tests half-open resource lifetimes.
 */

#include <Memory/RHIResourceLifetime.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIResourceLifetime, AdjacentIntervalsDoNotOverlap )
{
    LUMEN_CHECK_FALSE( ( FRHIResourceLifetime{ 0ULL, 2ULL }.Overlaps( { 2ULL, 4ULL } ) ) );
    LUMEN_CHECK_TRUE( ( FRHIResourceLifetime{ 0ULL, 3ULL }.Overlaps( { 2ULL, 4ULL } ) ) );
    LUMEN_CHECK_FALSE( ( FRHIResourceLifetime{ 4ULL, 3ULL }.IsValid() ) );
}

} // namespace Lumen
