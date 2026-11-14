/**
 * @file LaunchFrameTimingTests.cpp
 * @brief Functional tests for FFrameClock and FFrameLimiter.
 */

#include <Engine/LaunchFrameTiming.hpp>

#include <HAL/PlatformTime.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Public
 */

LUMEN_TEST( FFrameClock_Construct, UsesProvidedParameters )
{
    const Launch::FFrameClock Clock( 0.016, 0.1, 0.0 );

    LUMEN_CHECK_TRUE( Clock.ElapsedSeconds() >= 0.0 );
}

LUMEN_TEST( FFrameClock_BeginFrame, ComputesSingleFixedStep )
{
    Launch::FFrameClock Clock( 0.016, 0.1, 0.0 );

    const Launch::FFrameStep Step = Clock.BeginFrame( 0.02 );

    LUMEN_CHECK_EQ( Step.FixedSteps, 1U );
    LUMEN_CHECK_TRUE( Step.DeltaSeconds == 0.02 );
    LUMEN_CHECK_TRUE( Step.Alpha >= 0.0 );
    LUMEN_CHECK_TRUE( Step.Alpha < 1.0 );
}

LUMEN_TEST( FFrameClock_BeginFrame, AccumulatesMultipleFixedSteps )
{
    Launch::FFrameClock Clock( 0.01, 0.1, 0.0 );

    const Launch::FFrameStep Step = Clock.BeginFrame( 0.035 );

    LUMEN_CHECK_EQ( Step.FixedSteps, 3U );
}

LUMEN_TEST( FFrameClock_BeginFrame, ClampsDeltaToMaxFrameTime )
{
    Launch::FFrameClock Clock( 0.01, 0.05, 0.0 );

    const Launch::FFrameStep Step = Clock.BeginFrame( 1.0 );

    LUMEN_CHECK_TRUE( Step.DeltaSeconds <= 0.05 );
}

LUMEN_TEST( FFrameClock_BeginFrame, AlphaIsNormalizedRemainder )
{
    constexpr Float64 FixedStep = 0.01;
    Launch::FFrameClock Clock( FixedStep, 0.1, 0.0 );

    const Launch::FFrameStep Step = Clock.BeginFrame( 0.025 );

    LUMEN_CHECK_EQ( Step.FixedSteps, 2U );
    LUMEN_CHECK_TRUE( Step.Alpha >= 0.0 );
    LUMEN_CHECK_TRUE( Step.Alpha < 1.0 );
}

LUMEN_TEST( FFrameClock_BeginFrame, ClampsTimeRegressionToZero )
{
    Launch::FFrameClock Clock( 0.016, 0.1, 1.0 );

    const Launch::FFrameStep Step = Clock.BeginFrame( 0.5 );

    LUMEN_CHECK_TRUE( Step.DeltaSeconds == 0.0 );
    LUMEN_CHECK_EQ( Step.FixedSteps, 0U );
    LUMEN_CHECK_TRUE( Step.Alpha == 0.0 );
}

LUMEN_TEST( FFrameClock_BeginFrame, CarriesAccumulatorAcrossFrames )
{
    Launch::FFrameClock Clock( 0.01, 0.1, 0.0 );

    const Launch::FFrameStep FirstStep  = Clock.BeginFrame( 0.025 );
    const Launch::FFrameStep SecondStep = Clock.BeginFrame( 0.025 );

    LUMEN_CHECK_EQ( FirstStep.FixedSteps, 2U );
    LUMEN_CHECK_TRUE( FirstStep.Alpha >= 0.0 );
    LUMEN_CHECK_TRUE( FirstStep.Alpha < 1.0 );

    /** INFO: No new time elapsed: the leftover accumulator must not produce a step again. */
    LUMEN_CHECK_TRUE( SecondStep.DeltaSeconds == 0.0 );
    LUMEN_CHECK_EQ( SecondStep.FixedSteps, 0U );
    LUMEN_CHECK_EQ( SecondStep.Alpha, FirstStep.Alpha );
}

LUMEN_TEST( FFrameClock_ElapsedSeconds, GrowsWithTime )
{
    Launch::FFrameClock Clock( 0.016, 0.1 );

    const Float64 First = Clock.ElapsedSeconds();
    HAL::FPlatformTime::Sleep( 0.02 );
    const Float64 Second = Clock.ElapsedSeconds();

    LUMEN_CHECK_TRUE( Second > First );
}

LUMEN_TEST( FFrameLimiter_Construct, UncappedReturnsImmediately )
{
    Launch::FFrameLimiter Limiter( 0.0 );

    const Float64 Start = HAL::FPlatformTime::Seconds();
    Limiter.Wait();
    const Float64 End = HAL::FPlatformTime::Seconds();

    LUMEN_CHECK_TRUE( ( End - Start ) < 0.01 );
}

LUMEN_TEST( FFrameLimiter_Wait, RespectsBudget )
{
    Launch::FFrameLimiter Limiter( 60.0 );

    const Float64 Start = HAL::FPlatformTime::Seconds();
    Limiter.Wait();
    Limiter.Wait();
    const Float64 End = HAL::FPlatformTime::Seconds();

    LUMEN_CHECK_TRUE( ( End - Start ) >= 0.01 );
}

LUMEN_TEST( FFrameStep, CopyPreservesValues )
{
    const Launch::FFrameStep Step{ .DeltaSeconds = 0.016, .FixedSteps = 1U, .Alpha = 0.5 };
    const Launch::FFrameStep Copy = Step;

    LUMEN_CHECK_TRUE( Copy.DeltaSeconds == 0.016 );
    LUMEN_CHECK_EQ( Copy.FixedSteps, 1U );
    LUMEN_CHECK_TRUE( Copy.Alpha == 0.5 );
}

} // namespace Lumen
