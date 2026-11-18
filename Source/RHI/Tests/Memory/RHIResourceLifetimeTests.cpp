/**
 * @file RHIResourceLifetimeTests.cpp
 * @brief Tests half-open resource lifetimes.
 */

#include <RHI/Memory/RHIResourceLifetime.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIResourceLifetime, AdjacentIntervalsDoNotOverlap )
    {
        LUMEN_CHECK_FALSE( ( FRHIResourceLifetime{ 0ULL, 2ULL }.Overlaps( { 2ULL, 4ULL } ) ) );
        LUMEN_CHECK_TRUE( ( FRHIResourceLifetime{ 0ULL, 3ULL }.Overlaps( { 2ULL, 4ULL } ) ) );
        LUMEN_CHECK_FALSE( ( FRHIResourceLifetime{ 4ULL, 3ULL }.IsValid() ) );
    }

} // namespace RHI

} // namespace Lumen
