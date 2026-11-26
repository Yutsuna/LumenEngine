/**
 * @file OpenGLContext.hpp
 * @brief Declares the single-threaded OpenGL executor and native context owner.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Memory/SharedPtr.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Loader/GLDispatchTable.hpp>
#include <RHIOpenGL/Platform/Linux/EGLPlatformContext.hpp>

#include <Templates/ConditionVariable.hpp>
#include <Templates/Function.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Queue.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLContext
     * @brief Owns one EGL context on one dedicated worker thread.
     * @details Every native OpenGL call must be made from this worker. Synchronous
     *          callers can post a task with InvokeOnGLThread; it must never be called
     *          while holding another ranked RHI lock.
     */
    class FOpenGLContext final : public FNonCopyable, public FNonMovable
    {
    public:

        /** @brief Constructs an unstarted context; prefer Create() for normal use. */
        explicit FOpenGLContext ( Bool InEnableDebug ) noexcept;

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

    private:

        [[nodiscard]] RHI::FRHIExpected Start () noexcept;
        void Worker ( FStopToken InStopToken ) noexcept;

    private:

        Bool bEnableDebug = false;
        FEGLPlatformContext NativeContext;
        FGLDispatchTable Dispatch;

        FThread WorkerThread;
        HAL::FPlatformMutex QueueMutex;
        FConditionVariable Wake;
        TQueue<TFunction<void()>> Tasks;
        TOptional<RHI::FRHIError> InitError;
        Bool bReady = false;
    };

} // namespace RHIOpenGL

} // namespace Lumen
