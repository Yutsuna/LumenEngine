/**
 * @file IRHIQueue.hpp
 * @brief Declares serialized queue submission and host waits.
 */

#pragma once

#include <Queue/RHISubmitDesc.hpp>

namespace Lumen
{
/**
 * @class IRHIQueue
 * @brief A queue that accepts command submissions and exposes host waits.
 * @details A queue is confined to one family and type for its lifetime, as
 *          reported by GetType. Submission to one queue is serialized
 *          internally: callers may submit from multiple threads without
 *          external locking, but submission order follows lock acquisition.
 *          Different queues may be used concurrently when their synchronization
 *          descriptors make resource access legal. The queue retains every
 *          command list and semaphore it references until the corresponding
 *          timeline value completes. Timeline-based waits are gated by the
 *          `TimelineSemaphores` capability; a backend that does not advertise
 *          it must reject descriptor validation with `Unsupported`. No queue
 *          operation throws, and all failures are returned as FRHIError values.
 */
class IRHIQueue : public IRHIObject
{
public:

    /**
     * @brief Reports the family and type of work this queue accepts.
     * @return The queue type fixed at creation.
     */
    [[nodiscard]] virtual ERHIQueueType GetType () const noexcept = 0;

    /**
     * @brief Reports the most recent completed timeline value.
     * @return A value that never regresses for this queue's lifetime.
     */
    [[nodiscard]] virtual FRHITimelineValue GetCompletedValue () const noexcept = 0;

    /**
     * @brief Submits command lists and semaphore operations to this queue.
     * @param InDesc Command lists, waits, signals and a completion value; the
     *        descriptor is validated and its referenced objects are retained by
     *        the queue until completion.
     * @return Success when the submission is queued; InvalidArgument when
     *         InDesc fails validation, Unsupported for a negotiated-out feature,
     *         or DeviceLost / DeviceClosed after device loss or closure.
     */
    virtual TExpected<void, FRHIError> Submit ( const FRHISubmitDesc &InDesc ) noexcept = 0;

    /**
     * @brief Blocks until all work submitted to this queue has completed.
     * @return Success when the queue is idle, or DeviceLost / DeviceClosed if
     *         the device is no longer usable. Polling and cleanup stay available.
     */
    virtual TExpected<void, FRHIError> WaitIdle () noexcept = 0;

    /**
     * @brief Waits until a timeline value is reached or a timeout elapses.
     * @param InValue Timeline value to wait for.
     * @param InTimeoutSeconds Nonnegative timeout in seconds; zero polls.
     * @return Success when InValue is reached; Timeout if the wait elapses first,
     *         InvalidArgument for a negative timeout, or DeviceLost / DeviceClosed
     *         if the device is lost while waiting.
     */
    virtual TExpected<void, FRHIError> Wait ( FRHITimelineValue InValue, Float64 InTimeoutSeconds ) noexcept = 0;
};

} // namespace Lumen
