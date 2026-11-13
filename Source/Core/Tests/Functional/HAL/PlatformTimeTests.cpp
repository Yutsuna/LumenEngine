/**
 * @file PlatformTimeTests.cpp
 * @brief Functional tests for FPlatformTime.
 */

#include <Tests/UnitTest.hpp>

#include <HAL/PlatformTime.hpp>

namespace Lumen
{

LUMEN_TEST( FPlatformTime_Seconds, StartsNearZero )
{
    const Float64 Now = HAL::FPlatformTime::Seconds();

    /**
     * INFO: "since the application started". it cannot be negative, and a CI run never
     *        lives for more than a day, so any value within that window is coherent.
     */
    LUMEN_CHECK_TRUE( Now >= 0.0 );
    LUMEN_CHECK_TRUE( Now < 86400.0 );
}

LUMEN_TEST( FPlatformTime_Seconds, IsMonotonic )
{
    const Float64 First  = HAL::FPlatformTime::Seconds();
    const Float64 Second = HAL::FPlatformTime::Seconds();
    const Float64 Third  = HAL::FPlatformTime::Seconds();

    LUMEN_CHECK_TRUE( Second >= First );
    LUMEN_CHECK_TRUE( Third >= Second );
}

LUMEN_TEST( FPlatformTime_Seconds, AdvancesDuringSleep )
{
    const Float64 Start = HAL::FPlatformTime::Seconds();
    HAL::FPlatformTime::Sleep( 0.05 );
    const Float64 End = HAL::FPlatformTime::Seconds();

    LUMEN_CHECK_TRUE( End > Start );
    LUMEN_CHECK_TRUE( ( End - Start ) >= 0.03 );
}

LUMEN_TEST( FPlatformTime_NowNanoseconds, IsUnixEpochBased )
{
    const UInt64 Nanos = HAL::FPlatformTime::NowNanoseconds();

    /**
     * INFO: unix epoch in ns 1.6e18 ≈ Sept 2020, 5.0e18 ≈ year 2128.
     *        Any CI run lands squarely inside this window.
     */
    LUMEN_CHECK_TRUE( Nanos > 1'600'000'000'000'000'000ULL );
    LUMEN_CHECK_TRUE( Nanos < 5'000'000'000'000'000'000ULL );
}

LUMEN_TEST( FPlatformTime_NowNanoseconds, AdvancesDuringSleep )
{
    const UInt64 Start = HAL::FPlatformTime::NowNanoseconds();
    HAL::FPlatformTime::Sleep( 0.05 );
    const UInt64 End = HAL::FPlatformTime::NowNanoseconds();

    LUMEN_CHECK_TRUE( End > Start );

    const UInt64 ElapsedNanos = End - Start;
    LUMEN_CHECK_TRUE( ElapsedNanos >= 30000000ULL );
}

LUMEN_TEST( FPlatformTime_Sleep, ZeroIsSafe )
{
    HAL::FPlatformTime::Sleep( 0.0 );
    LUMEN_CHECK_TRUE( true );
}

LUMEN_TEST( FPlatformTime_Constants, TickRatesAreCoherent )
{
    /**
     * INFO: DefaultTickRate = 1/60 (60 FPS), MaxTickRate = 1/15 (≈ 15 FPS floor).
     *        The MaxTickRate is the upper bound on delta-seconds, i.e. a lower bound on FPS.
     */
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::DefaultTickRate > 0.0 );
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::DefaultTickRate < 1.0 );
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::MaxTickRate > 0.0 );
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::MaxTickRate < 1.0 );
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::DefaultTickRate < HAL::FPlatformTime::MaxTickRate );

    /** 1/60 ≈ 0.016667 */
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::DefaultTickRate > 0.015 );
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::DefaultTickRate < 0.018 );

    /** 1/15 ≈ 0.066667 */
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::MaxTickRate > 0.065 );
    LUMEN_CHECK_TRUE( HAL::FPlatformTime::MaxTickRate < 0.070 );
}

} // namespace Lumen
