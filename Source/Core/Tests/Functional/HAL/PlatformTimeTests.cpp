/**
 * @file PlatformTimeTests.cpp
 * @brief Functional tests for HAL::FPlatformTime.
 */

#include <HAL/PlatformTime.hpp>
#include <Tests/UnitTest.hpp>

#include <chrono>

using namespace std::chrono_literals;

namespace Lumen
{

/** @return True when the two durations are within the given absolute tolerance. */
static Bool WithinTolerance ( Float64 Actual, Float64 Expected, Float64 Tolerance )
{
    const Float64 Difference = Actual > Expected ? Actual - Expected : Expected - Actual;
    return Difference <= Tolerance;
}

/**
 * constants (compile-time)
 */

static_assert( HAL::FPlatformTime::DefaultTickRate == 1.0 / 60.0, "DefaultTickRate must be 1/60" );
static_assert( HAL::FPlatformTime::MaxTickRate == 1.0 / 15.0, "MaxTickRate must be 1/15" );

/**
 * Seconds()
 */

LUMEN_TEST( PlatformTime_Seconds, NonNegative )
{
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::Seconds() >= 0.0 );
}

LUMEN_TEST( PlatformTime_Seconds, MonotonicBetweenCalls )
{
    const Float64 First  = HAL::FPlatformTime::Seconds();
    const Float64 Second = HAL::FPlatformTime::Seconds();
    const Float64 Third  = HAL::FPlatformTime::Seconds();

    LUMEN_CHECK_TRUE( Second >= First );
    LUMEN_CHECK_TRUE( Third >= Second );
}

LUMEN_TEST( PlatformTime_Seconds, AdvancesAfterSleep )
{
    /** INFO: settle first so the delta is not obscured by scheduler noise */
    HAL::FPlatformTime::Sleep( 0.02 );

    const Float64 Before = HAL::FPlatformTime::Seconds();

    HAL::FPlatformTime::Sleep( 0.05 );

    const Float64 After = HAL::FPlatformTime::Seconds();

    LUMEN_CHECK_TRUE( After - Before >= 0.045 );
    LUMEN_CHECK_TRUE( WithinTolerance( After - Before, 0.05, 0.05 ) );
}

/**
 * NowNanoseconds()
 */

LUMEN_TEST( PlatformTime_NowNanoseconds, ReturnsPositiveEpochValue )
{
    /**
     * INFO: 1'483'228'800 nanoseconds-first epoch is 2017-01-01; the wall
     * clock must at least be past that by now.
     */
    static constexpr UInt64 Year2017Nanos = 1483228800000000000ULL;

    const UInt64 Now = HAL::FPlatformTime::NowNanoseconds();

    LUMEN_CHECK_TRUE( Now > Year2017Nanos );
}

LUMEN_TEST( PlatformTime_NowNanoseconds, NearlyMonotonicBetweenCalls )
{
    /**
     * INFO: system_clock may slew backwards a hair under NTP; allow up to
     * 1 ms of regression, but never more.
     */
    static constexpr UInt64 ClockSlewToleranceNanos = 1'000'000ULL;

    const UInt64 First  = HAL::FPlatformTime::NowNanoseconds();
    const UInt64 Second = HAL::FPlatformTime::NowNanoseconds();

    LUMEN_CHECK_TRUE( Second + ClockSlewToleranceNanos > First );

    const UInt64 Delta = Second >= First ? Second - First : 0ULL;
    LUMEN_CHECK_TRUE( Delta < 1'000'000'000ULL );
}

LUMEN_TEST( PlatformTime_NowNanoseconds, AdvancesDuringSleep )
{
    static constexpr UInt64 MinimumAdvanceNanos = 15'000'000ULL;

    const UInt64 Before = HAL::FPlatformTime::NowNanoseconds();

    HAL::FPlatformTime::Sleep( 0.02 );

    const UInt64 After = HAL::FPlatformTime::NowNanoseconds();

    LUMEN_CHECK_TRUE( After >= Before + MinimumAdvanceNanos );
}

/**
 * Sleep()
 */

LUMEN_TEST( PlatformTime_Sleep, BlocksForRequestedDuration )
{
    const std::chrono::steady_clock::time_point Start = std::chrono::steady_clock::now();

    HAL::FPlatformTime::Sleep( 0.025 );

    const std::chrono::steady_clock::time_point End = std::chrono::steady_clock::now();
    const Float64 Elapsed                           = std::chrono::duration<Float64>( End - Start ).count();

    LUMEN_CHECK_TRUE( Elapsed >= 0.024 );
}

LUMEN_TEST( PlatformTime_Sleep, ZeroDurationDoesNotBlock )
{
    const std::chrono::steady_clock::time_point Start = std::chrono::steady_clock::now();

    HAL::FPlatformTime::Sleep( 0.0 );

    const std::chrono::steady_clock::time_point End = std::chrono::steady_clock::now();
    const Float64 Elapsed                           = std::chrono::duration<Float64>( End - Start ).count();

    LUMEN_CHECK_TRUE( Elapsed < 0.05 );
}

LUMEN_TEST( PlatformTime_Sleep, SmallDurationStillYieldsProgress )
{
    /** INFO: two tiny sleeps must still be reflected in monotonic time */
    const Float64 Before = HAL::FPlatformTime::Seconds();

    HAL::FPlatformTime::Sleep( 0.01 );
    HAL::FPlatformTime::Sleep( 0.01 );

    const Float64 After = HAL::FPlatformTime::Seconds();

    LUMEN_CHECK_TRUE( After >= Before );
}

/**
 * tick-rate constants (runtime re-check)
 */

LUMEN_TEST( PlatformTime_TickRates, ConstantsMatchDocumentation )
{
    LUMEN_CHECK_TRUE( WithinTolerance( HAL::FPlatformTime::DefaultTickRate, 1.0 / 60.0, 1e-12 ) );
    LUMEN_CHECK_TRUE( WithinTolerance( HAL::FPlatformTime::MaxTickRate, 1.0 / 15.0, 1e-12 ) );
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::DefaultTickRate > 0.0 );
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::DefaultTickRate < HAL::FPlatformTime::MaxTickRate );
}

} // namespace Lumen
