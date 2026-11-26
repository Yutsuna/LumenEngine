/**
 * @file OpenGLQueue.hpp
 * @brief Declares the logical OpenGL queue and its synchronous command replay.
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
     * @brief Logical queue that serially replays command streams on the GL executor.
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

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::ERHIQueueType QueueType = RHI::ERHIQueueType::Graphics;
        UInt32 QueueIndex            = 0U;
        TAtomic<UInt64> CompletedValue{ 0ULL };
        HAL::FPlatformMutex SubmitMutex;
        TSharedPtr<FOpenGLCommandExecutor> Executor;
    };

} // namespace RHIOpenGL

} // namespace Lumen
