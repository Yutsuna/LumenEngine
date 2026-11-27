/**
 * @file OpenGLContext.hpp
 * @brief Declares the single-threaded OpenGL executor, submission scheduler and debug bridge.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Memory/SharedPtr.hpp>
#include <RHI/Debug/IRHIDebugSink.hpp>
#include <RHI/Debug/RHIDebugMessage.hpp>
#include <RHI/Synchronization/IRHISemaphore.hpp>
#include <RHI/Synchronization/RHIDeferredReleaseQueue.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHITimeline.hpp>

#include <RHIOpenGL/Loader/GLDispatchTable.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>
#include <RHIOpenGL/Platform/Linux/EGLPlatformContext.hpp>

#include <Templates/ConditionVariable.hpp>
#include <Templates/Function.hpp>
#include <Templates/Map.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Pair.hpp>
#include <Templates/Queue.hpp>
#include <Templates/Vector.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @struct FOpenGLWaitState
     * @brief Shared leaf lock and condition variable for host-side queue waits.
     * @details Rank 7 leaf: no other lock may be acquired while it is held. All logical
     *          queues share one GL executor, so one shared wait state is sufficient;
     *          waiters re-evaluate their own queue value after every notification.
     */
    struct FOpenGLWaitState
    {
        HAL::FPlatformMutex Mutex;
        FConditionVariable Condition;
        Bool bUnavailable = false;
    };

    /**
     * @struct FOpenGLSubmission
     * @brief Immutable async replay packet retained by the GL executor until fence completion.
     */
    struct FOpenGLSubmission
    {
        TFunction<RHI::FRHIExpected()> Replay;
        TVector<TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64>> Waits;
        TVector<TPair<TSharedPtr<RHI::IRHISemaphore>, UInt64>> Signals;
        RHI::FRHITimelineValue Completion;
        TFunction<void()> OnComplete;
        TFunction<void( RHI::FRHIError )> OnFailure;
    };

    /**
     * @class FOpenGLContext
     * @brief Owns one EGL context on one dedicated worker thread.
     * @details Every native OpenGL call must be made from this worker. Synchronous
     *          callers can post a task with InvokeOnGLThread; it must never be called
     *          while holding any of the six ranked RHI locks. Asynchronous submissions
     *          are released only after a GLsync fence signals.
     */
    class FOpenGLContext final : public FNonCopyable, public FNonMovable
    {
    public:

        /** @brief Constructs an unstarted context; prefer Create() for normal use. */
        explicit FOpenGLContext ( Bool bDebugEnabled ) noexcept;

        /** @brief Creates the context and waits until its worker is ready. */
        [[nodiscard]] static TExpected<TSharedRef<FOpenGLContext>, RHI::FRHIError> Create ( Bool bEnableDebug ) noexcept;

        ~FOpenGLContext () noexcept;

        /**
         * @brief Stops the executor and retires deferred releases on it.
         * @details Idempotent. Drain and shutdown are posted before the stop is published so
         *          every accepted task remains observable by the worker. After this returns,
         *          ExecuteOnGLThread/InvokeOnGLThread fail fast with DeviceLost.
         */
        void Shutdown () noexcept;

        /** @return Resolved OpenGL 4.6 entry points for the worker context. */
        [[nodiscard]] const FGLDispatchTable &GetDispatch () const noexcept;

        /** @return Highest timeline value published by the executor after fence completion. */
        [[nodiscard]] RHI::FRHITimelineValue GetCompletedTimeline () const noexcept;

        /** @return Address of the leaf wait state owned by this context. */
        [[nodiscard]] FOpenGLWaitState *GetWaitState () noexcept;

        /** @brief Runs a task on the GL Executor; inline when already on that thread. */
        [[nodiscard]] RHI::FRHIExpected ExecuteOnGLThread ( TFunction<void()> InTask ) noexcept;

        /** @return True when the caller is the GL Executor thread. */
        [[nodiscard]] Bool IsExecutorThread () const noexcept;

        /** @brief Posts a task to the worker and waits for its completion. */
        [[nodiscard]] RHI::FRHIExpected InvokeOnGLThread ( TFunction<void()> InTask ) noexcept;

        /** @brief Enqueues non-blocking replay work; completion is fence-driven. */
        [[nodiscard]] RHI::FRHIExpected Submit ( FOpenGLSubmission &&InSubmission ) noexcept;

        /** @brief Attaches the non-owning debug sink. */
        void SetDebugSink ( RHI::IRHIDebugSink *InSink ) noexcept;

        /** @brief Records a graphics reset observed on the GL executor. */
        void MarkDeviceLost () noexcept;

        /**
         * @brief Retires every unfinished submission on a clean device closure.
         * @details Posts the retirement to the GL Executor (inline when already there), then
         *          wakes host waiters. Terminal callbacks therefore run before waiters are
         *          released, preserving the single-terminal ordering (D7). If the executor is
         *          already stopped, its stop path has retired the work and this only wakes
         *          waiters. No GL call is made from the caller thread.
         */
        void MarkDeviceClosed () noexcept;

        /** @return True after a graphics reset was observed on the GL executor. */
        [[nodiscard]] Bool IsDeviceLost () const noexcept;

        /** @brief Forwards an already-owned debug message to the attached sink. */
        void EmitDebugMessage ( RHI::FRHIDebugMessage InMessage ) noexcept;

        /**
         * @brief Retires a GL-thread release when the tracked timeline completes.
         * @details Must never be called under one of the six ranked locks. When the required
         *          value is already reached the release runs synchronously on the executor;
         *          otherwise it is queued and retired by the worker after fence completion.
         *          The callback contains native GL calls and always runs on the executor.
         */
        [[nodiscard]] RHI::FRHIExpected DeferRelease ( RHI::FRHITimelineValue InRequired, TFunction<void()> InRelease ) noexcept;

        /** @brief Registers a query-result poll that runs on the GL Executor. */
        void TrackQueryPoll ( TFunction<Bool()> InPoll ) noexcept;

        /** @brief Publishes every query result that became available. */
        void PollPendingQueries () noexcept;

        /** @brief Native KHR_debug thunk entry; copies text under the leaf lock only. */
        void NotifyDebugMessage ( GLenum InSource,
                                  GLenum InType,
                                  GLenum InSeverity,
                                  UInt32 InCode,
                                  GLsizei InLength,
                                  const AnsiChar *InText ) const noexcept;

    private:

        struct FActiveFence
        {
            GLsync Sync = nullptr;
            FOpenGLSubmission Submission;
        };

        [[nodiscard]] RHI::FRHIExpected Start () noexcept;
        void Worker ( FStopToken InStopToken ) noexcept;
        void StepScheduler () noexcept;
        void PollResetStatus () noexcept;
        /** @brief Publishes host-side unavailability and wakes every wait state waiter. */
        void PublishUnavailable () noexcept;
        /**
         * @brief Fails every unfinished submission with InError; executor thread only.
         * @details Drains Pending/Blocked/ActiveFences under SchedulerMutex, deletes native
         *          sync objects on the executor, then invokes OnFailure outside the lock.
         *          No timeline value is published here: per-queue completion is published
         *          by the queue's OnFailure callback.
         */
        void FailAllSubmissions ( const RHI::FRHIError &InError ) noexcept;

        /**
         * @brief Publishes exactly one terminal callback for a retired submission.
         * @details Executor thread only. Success signals the output semaphores and calls
         *          OnComplete; failure calls OnFailure and never signals. The executor
         *          timeline and the deferred-release queue advance on both paths.
         */
        void RetireSubmission ( FOpenGLSubmission &&InSubmission, const RHI::FRHIExpected &InResult ) noexcept;

    private:

        Bool bEnableDebug = false;
        FEGLPlatformContext NativeContext;
        FGLDispatchTable Dispatch;
        TAtomic<Bool> bResetReported{ false };

        FThread WorkerThread;
        /** @brief Published by the worker at startup; safe to read from any thread during shutdown. */
        TAtomic<FThread::FId> ExecutorThreadId;
        HAL::FPlatformMutex QueueMutex;
        FConditionVariable Wake;
        TQueue<TFunction<void()>> Tasks;
        /** @brief Published under QueueMutex before stop; rejects new invokes atomically. */
        TAtomic<Bool> bStopping{ false };
        TOptional<RHI::FRHIError> InitError;
        Bool bReady = false;

        /** Rank 3: submission queue and active fences; never held across GL calls. */
        HAL::FPlatformMutex SchedulerMutex;
        TQueue<FOpenGLSubmission> Pending;
        TVector<FOpenGLSubmission> Blocked;
        TVector<FActiveFence> ActiveFences;
        RHI::FRHIDeferredReleaseQueue DeferredReleases;
        TVector<TFunction<Bool()>> PendingQueryPolls;
        TAtomic<UInt64> CompletedTimeline{ 0ULL };
        FOpenGLWaitState WaitState;

        /** Rank 6: leaf lock protecting the non-owning sink and its deduplication map. */
        mutable HAL::FPlatformMutex DebugSinkMutex;
        RHI::IRHIDebugSink *DebugSink = nullptr;
        mutable TMap<UInt32, Float64> RecentMessages;
        TAtomic<Bool> bDeviceLost{ false };
    };

} // namespace RHIOpenGL

} // namespace Lumen
