/**
 * @file RHITimeline.cpp
 * @brief Implements timeline completion comparisons.
 */

#include <Types/RHITimeline.hpp>

Lumen::Bool Lumen::IsRHICompletionReached ( const FRHITimelineValue InCompleted, const FRHITimelineValue InRequired ) noexcept
{
    return not InRequired.IsValid() or InCompleted.Value >= InRequired.Value;
}
