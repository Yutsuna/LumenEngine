/**
 * @file IRHIQueue.hpp
 * @brief Declares serialized queue submission and host waits.
 */

#pragma once
#include <Queue/RHISubmitDesc.hpp>
namespace Lumen
{
class IRHIQueue : public IRHIObject
{
public:

    [[nodiscard]] virtual ERHIQueueType GetType () const noexcept                                            = 0;
    [[nodiscard]] virtual FRHITimelineValue GetCompletedValue () const noexcept                              = 0;
    virtual TExpected<void, FRHIError> Submit ( const FRHISubmitDesc &InDesc ) noexcept                      = 0;
    virtual TExpected<void, FRHIError> WaitIdle () noexcept                                                  = 0;
    virtual TExpected<void, FRHIError> Wait ( FRHITimelineValue InValue, Float64 InTimeoutSeconds ) noexcept = 0;
};
} // namespace Lumen
