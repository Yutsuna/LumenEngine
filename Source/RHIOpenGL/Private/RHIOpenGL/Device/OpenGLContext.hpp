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

        /** @return Resolved OpenGL 4.6 entry points for the worker context. */
        [[nodiscard]] const FGLDispatchTable &GetDispatch () const noexcept;

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

        /** @return True after a graphics reset was observed on the GL executor. */
        [[nodiscard]] Bool IsDeviceLost () const noexcept;

        /** @brief Forwards an already-owned debug message to the attached sink. */
        void EmitDebugMessage ( RHI::FRHIDebugMessage InMessage ) noexcept;

        /** @brief Retires a GL-thread release when the tracked timeline completes. */
        [[nodiscard]] RHI::FRHIExpected DeferRelease ( RHI::FRHITimelineValue InRequired, TFunction<void()> InRelease ) noexcept;

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

    private:

        Bool bEnableDebug = false;
        FEGLPlatformContext NativeContext;
        FGLDispatchTable Dispatch;
        TAtomic<Bool> bResetReported{ false };

        FThread WorkerThread;
        HAL::FPlatformMutex QueueMutex;
        FConditionVariable Wake;
        TQueue<TFunction<void()>> Tasks;
        TOptional<RHI::FRHIError> InitError;
        Bool bReady = false;

        /** Rank 3: submission queue and active fences; never held across GL calls. */
        HAL::FPlatformMutex SchedulerMutex;
        TQueue<FOpenGLSubmission> Pending;
        TVector<FOpenGLSubmission> Blocked;
        TVector<FActiveFence> ActiveFences;
        RHI::FRHIDeferredReleaseQueue DeferredReleases;
        TAtomic<UInt64> CompletedTimeline{ 0ULL };

        /** Rank 6: leaf lock protecting the non-owning sink and its deduplication map. */
        mutable HAL::FPlatformMutex DebugSinkMutex;
        RHI::IRHIDebugSink *DebugSink = nullptr;
        mutable TMap<UInt32, Float64> RecentMessages;
        TAtomic<Bool> bDeviceLost{ false };
    };

} // namespace RHIOpenGL

} // namespace Lumen
