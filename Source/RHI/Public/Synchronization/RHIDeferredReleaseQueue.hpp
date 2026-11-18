/**
 * @file RHIDeferredReleaseQueue.hpp
 * @brief Declares timeline-gated thread-safe retirement.
 */

#pragma once

#include <HAL/PlatformMutex.hpp>
#include <RHIExport.hpp>
#include <Templates/Function.hpp>
#include <Templates/Vector.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHITimeline.hpp>

namespace Lumen
{

/**
 * @class FRHIDeferredReleaseQueue
 * @brief Thread-safe queue for deferred release of resources.
 */
class RHI_EXPORT FRHIDeferredReleaseQueue final
{
public:

    /** @brief Creates an empty queue with no retirement entries. */
    FRHIDeferredReleaseQueue () noexcept = default;

    /** @brief Drains remaining callbacks before destruction. */
    ~FRHIDeferredReleaseQueue () noexcept;

    /** @brief Takes ownership of a callback until completion or Drain. */
    TExpected<void, FRHIError> Enqueue ( FRHITimelineValue InRequired, TFunction<void()> InRelease ) noexcept;

    /** @brief Invokes completed callbacks outside the queue lock and returns their count. */
    USize Poll ( FRHITimelineValue InCompleted ) noexcept;

    /** @brief Invokes every remaining callback in FIFO order. */
    USize Drain () noexcept;

    /** @brief Drains and rejects subsequent Enqueue calls. */
    void Shutdown () noexcept;

    /**
     * @brief Returns the number of entries awaiting completion.
     * @return Pending entry count.
     */
    [[nodiscard]] USize GetSize () const noexcept;

    /**
     * @brief Reports whether the queue has been shut down.
     * @return True after Shutdown; Enqueue is then rejected.
     */
    [[nodiscard]] Bool IsShutdown () const noexcept;

private:

    struct FEntry
    {
        FRHITimelineValue Required;
        TFunction<void()> Release;
    };

    TVector<FEntry> Entries;
    Bool bShutdown = false;
    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
};
} // namespace Lumen
