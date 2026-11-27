/**
 * @file RenderExecutor.hpp
 * @brief Dedicated single-thread render command executor.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>
#include <Execution/RenderFrameTypes.hpp>
#include <HAL/PlatformMutex.hpp>
#include <Memory/UniquePtr.hpp>
#include <Templates/ConditionVariable.hpp>
#include <Templates/Expected.hpp>
#include <Thread/Thread.hpp>
#include <Thread/TripleBuffer.hpp>

namespace Lumen
{

namespace Thread
{
    template <typename Type> class TMPSCQueue;
}

/** @struct FRenderExecutorDesc */
struct FRenderExecutorDesc
{
    USize CommandCapacity = 64ULL;
};

/** @enum ERenderExecutorError */
enum class ERenderExecutorError : UInt8
{
    InvalidCapacity,
    AlreadyRunning,
    NotRunning,
    QueueFull,
    QueueCreationFailed,
    ThreadCreationFailed,
};

/** @class IRenderCommandHandler */
class RENDERER_EXPORT IRenderCommandHandler
{
public:

    virtual ~IRenderCommandHandler () noexcept = default;

    /** @brief Executes one command on the render thread and updates its working status. */
    virtual void HandleRenderCommand ( const FRenderCommand &InCommand, FRenderFrameStatus &InOutStatus ) noexcept = 0;
};

/** @class FRenderExecutor */
class RENDERER_EXPORT FRenderExecutor final : public FNonCopyable, public FNonMovable
{
public:

    FRenderExecutor () noexcept;
    ~FRenderExecutor () noexcept;

    /** @brief Starts one render thread. The handler must outlive Stop/Join and the executor. */
    [[nodiscard]] TExpected<void, ERenderExecutorError> Start ( const FRenderExecutorDesc &InDesc,
                                                                IRenderCommandHandler &InHandler ) noexcept;
    /** @brief Enqueues a bounded POD command without allocating. */
    [[nodiscard]] TExpected<void, ERenderExecutorError> Enqueue ( const FRenderCommand &InCommand ) noexcept;
    /** @brief Requests termination after already queued commands have been drained. */
    void RequestStop () noexcept;
    /** @brief Joins the render thread and releases queue storage. */
    void Join () noexcept;

    [[nodiscard]] Bool IsRunning () const noexcept;
    /** @brief Promotes the latest status. Only one reader thread may call this function. */
    void SwapStatusBuffers () noexcept;
    [[nodiscard]] const FRenderFrameStatus &GetStatus () const noexcept;

private:

    void Run ( FStopToken InStopToken ) noexcept;
    void SignalWork () noexcept;
    void PublishStatus () noexcept;

private:

    TUniquePtr<Thread::TMPSCQueue<FRenderCommand>> Commands;
    FThread WorkerThread;
    IRenderCommandHandler *Handler = nullptr;

    HAL::FPlatformMutex WakeMutex;
    FConditionVariable WakeCondition;
    Bool bWorkAvailable = false;
    Bool bStopRequested = false;

    TAtomic<Bool> bRunning{ false };
    TAtomic<UInt64> RejectedCommandCount{ 0ULL };
    FRenderFrameStatus WorkingStatus{};
    Thread::TTripleBuffer<FRenderFrameStatus> StatusBuffer;
};

} // namespace Lumen
