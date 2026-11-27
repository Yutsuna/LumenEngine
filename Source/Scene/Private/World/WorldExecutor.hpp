/**
 * @file WorldExecutor.hpp
 * @brief Private bounded world task executor with CPU phase fences.
 */

#pragma once

#include <World/WorldTypes.hpp>

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>
#include <HAL/PlatformMutex.hpp>
#include <Memory/Span.hpp>
#include <Templates/ConditionVariable.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{

class AActor;

class FWorldExecutor final : public FNonCopyable, public FNonMovable
{
public:

    FWorldExecutor ( UInt32 InWorkerCount, USize InTaskCapacity ) noexcept;
    ~FWorldExecutor () noexcept;

    [[nodiscard]] Bool IsValid () const noexcept;

    [[nodiscard]] TExpected<FWorldPhaseCompletion, EWorldTickError> ExecutePhase ( TSpan<AActor *const> InActors,
                                                                                   const FWorldFrameContext &InContext,
                                                                                   Float64 InDeltaTime,
                                                                                   UInt32 InMaxMessages ) noexcept;

private:

    struct FTask
    {
        AActor *Actor = nullptr;
        FWorldFrameContext Context;
        Float64 DeltaTime  = 0.0;
        UInt32 MaxMessages = 0U;
    };

    void WorkerMain ( FStopToken InStopToken ) noexcept;

private:

    TVector<FTask> TaskRing;
    TVector<FThread> Workers;
    HAL::FPlatformMutex QueueMutex;
    FConditionVariable WorkAvailable;
    TAtomic<UInt32> RemainingTasks = { 0U };
    USize QueueHead                = 0ULL;
    USize QueueTail                = 0ULL;
    USize QueuedTaskCount          = 0ULL;
    UInt32 ExpectedWorkerCount     = 0U;
    Bool bStopping                 = false;
};

} // namespace Lumen
