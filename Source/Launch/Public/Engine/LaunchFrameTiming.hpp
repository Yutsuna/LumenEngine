///
/// @file LaunchFrameTiming.hpp
/// @brief Fixed step clock and frame limiter.
///

#pragma once

#include <Core/CoreTypes.hpp>
#include <type_traits>

namespace Lumen
{

namespace Launch
{
    ///
    /// @struct FFrameStep
    ///
    struct FFrameStep
    {
        Float64 DeltaSeconds = 0.0;
        UInt32 FixedSteps    = 0;
        Float64 Alpha        = 0.0;
    };

    static_assert( std::is_trivially_copyable_v<FFrameStep>, "FFrameStep is returned by value every frame." );

    ///
    /// @class FFrameClock
    ///
    class FFrameClock final
    {
    public:

        FFrameClock ( Float64 InFixedTimeStep, Float64 InMaxFrameTime ) noexcept;
        FFrameClock ( Float64 InFixedTimeStep, Float64 InMaxFrameTime, Float64 InStartSeconds ) noexcept;

    public:

        [[nodiscard]] FFrameStep BeginFrame () noexcept;
        [[nodiscard]] FFrameStep BeginFrame ( Float64 InNowSeconds ) noexcept;
        [[nodiscard]] Float64 ElapsedSeconds () const noexcept;

    private:

        Float64 FixedTimeStep;
        Float64 MaxFrameTime;
        Float64 StartTime;
        Float64 PreviousTime;
        Float64 Accumulator = 0.0;
    };

    ///
    /// @class FFrameLimiter
    ///
    class FFrameLimiter final
    {
    public:

        explicit FFrameLimiter ( Float64 InMaxFPS ) noexcept;

    public:

        void Wait () noexcept;

    private:

        Float64 FrameBudget;
        Float64 NextDeadline;
    };

} // namespace Launch

} // namespace Lumen
