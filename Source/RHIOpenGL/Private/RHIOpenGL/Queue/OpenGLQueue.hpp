/**
 * @file OpenGLQueue.hpp
 * @brief Declares the logical OpenGL queue with fence-driven async submission.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Queue/IRHIQueue.hpp>

#include <RHIOpenGL/Commands/OpenGLCommandExecutor.hpp>
#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLQueue
     * @brief Serializes submission and retires command lists only after GL fence completion.
     */
    class FOpenGLQueue final : public RHI::IRHIQueue, public FNonCopyable, public FNonMovable
    {
    public:

        /** @brief Factory kept in this TU so -O3 does not confuse controller layouts. */
        [[nodiscard]] static TSharedRef<FOpenGLQueue> Create ( RHI::FRHIObjectId InId,
                                                               TSharedRef<FOpenGLDeviceContext> InContext,
                                                               RHI::ERHIQueueType InType,
                                                               UInt32 InIndex ) noexcept;

        FOpenGLQueue ( RHI::FRHIObjectId InId,
                       TSharedRef<FOpenGLDeviceContext> InContext,
                       RHI::ERHIQueueType InType,
                       UInt32 InIndex ) noexcept;
        ~FOpenGLQueue () noexcept override = default;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] RHI::ERHIQueueType GetType () const noexcept override;
        [[nodiscard]] RHI::FRHITimelineValue GetCompletedValue () const noexcept override;

    public:

        RHI::FRHIExpected Submit ( const RHI::FRHISubmitDesc &InDesc ) noexcept override;
        RHI::FRHIExpected WaitIdle () noexcept override;
        RHI::FRHIExpected Wait ( RHI::FRHITimelineValue InValue, Float64 InTimeoutSeconds ) noexcept override;

    private:

        /**
         * @brief Creates the replay executor once, before @ref SubmitMutex is acquired.
         * @details Creation dispatches to the GL thread with `ExecuteOnGLThread`, which is
         *          forbidden while a ranked lock is held (module plan §1.4). @ref ExecutorMutex
         *          is an unranked leaf: it is never nested with a ranked lock and no GL-thread
         *          task ever acquires it. Publication of @ref Executor is therefore safe against
         *          concurrent first submissions.
         */
        [[nodiscard]] RHI::FRHIExpected EnsureExecutor () noexcept;

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::ERHIQueueType QueueType               = RHI::ERHIQueueType::Graphics;
        UInt32 QueueIndex                          = 0U;
        TSharedRef<TAtomic<UInt64>> CompletedValue = MakeShared<TAtomic<UInt64>>( 0ULL );
        TAtomic<UInt64> SubmittedValue{ 0ULL };
        HAL::FPlatformMutex SubmitMutex;   ///< Rank 1: submission serialization.
        HAL::FPlatformMutex ExecutorMutex; ///< Unranked leaf: one-time executor creation only.
        TSharedPtr<FOpenGLCommandExecutor> Executor;
    };

} // namespace RHIOpenGL

} // namespace Lumen
