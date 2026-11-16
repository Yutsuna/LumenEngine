/**
 * @file RHIDeferredReleaseQueue.hpp
 * @brief Declares timeline-gated thread-safe retirement.
 */

#pragma once
#include <HAL/PlatformMutex.hpp>
#include <Templates/Function.hpp>
#include <Templates/Vector.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHITimeline.hpp>
namespace Lumen
{
class RHI_EXPORT FRHIDeferredReleaseQueue final
{
public:

    FRHIDeferredReleaseQueue () noexcept = default;
    ~FRHIDeferredReleaseQueue () noexcept;
    /** @brief Takes ownership of a callback until completion or Drain. */
    TExpected<void, FRHIError> Enqueue ( FRHITimelineValue InRequired, TFunction<void()> InRelease ) noexcept;
    /** @brief Invokes completed callbacks outside the queue lock and returns their count. */
    USize Poll ( FRHITimelineValue InCompleted ) noexcept;
    /** @brief Invokes every remaining callback in FIFO order. */
    USize Drain () noexcept;
    /** @brief Drains and rejects subsequent Enqueue calls. */
    void Shutdown () noexcept;
    [[nodiscard]] USize GetSize () const noexcept;
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
