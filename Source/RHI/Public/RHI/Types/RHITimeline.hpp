/**
 * @file RHITimeline.hpp
 * @brief Defines timeline values and frame completion tokens.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>
#include <compare>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHITimelineValue
     * @brief Monotonic GPU completion value; zero means no work.
     */
    struct FRHITimelineValue
    {
        UInt64 Value = 0ULL;

        /** @return True when this value names submitted work. */
        [[nodiscard]] constexpr Bool IsValid () const noexcept
        {
            return Value != 0ULL;
        }
        friend constexpr auto operator<=>( const FRHITimelineValue &, const FRHITimelineValue & ) = default;
    };

    /**
     * @struct FRHIFrameToken
     * @brief Associates a CPU frame index with GPU completion.
     */
    struct FRHIFrameToken
    {
        UInt64 FrameIndex            = 0ULL;
        FRHITimelineValue Completion = {};
    };

    /** @return True when Completed reaches or exceeds Required. */
    [[nodiscard]] RHI_EXPORT Bool IsRHICompletionReached ( FRHITimelineValue InCompleted, FRHITimelineValue InRequired ) noexcept;

} // namespace RHI

} // namespace Lumen
