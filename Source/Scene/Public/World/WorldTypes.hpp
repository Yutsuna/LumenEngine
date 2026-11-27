/**
 * @file WorldTypes.hpp
 * @brief World execution configuration and phase completion contracts.
 */

#pragma once

#include <Actor/ActorLifecycle.hpp>
#include <SceneExport.hpp>

#include <Rendering/RenderSnapshotTypes.hpp>

namespace Lumen
{

/**
 * @enum EWorldTickError
 * @brief Structured world execution capacity failures.
 */
enum class EWorldTickError : UInt8
{
    ExecutorUnavailable,
    PhaseCapacityExceeded,
    EmissionCapacityExceeded,
    FixedStepLimitExceeded,
    WorldCapacityExceeded,
};

/**
 * @struct FWorldDesc
 * @brief Fixed capacities and worker configuration of a world.
 */
struct SCENE_EXPORT FWorldDesc
{
    UInt32 WorkerCount            = 1U;
    UInt32 MaxFixedStepsPerTick   = 8U;
    USize MaxActors               = 10000ULL;
    USize DeferredCommandCapacity = 1024ULL;
    USize ActorOutboxCapacity     = 16ULL;
    USize EmissionCapacity        = 4096ULL;

    /**
     * @brief Fixed hierarchy node capacity; zero leaves the spatial hierarchy disabled.
     * @details Reserved once at World construction. Registration is opt-in so existing worlds keep their M0-M7 behaviour.
     */
    USize HierarchyCapacity = 0ULL;

    Float64 FixedDeltaTime = 0.0;

    /** @brief Explicit world identity; zero asks FWorld to allocate a process-unique non-zero ID. */
    UInt64 WorldId = 0ULL;

    /**
     * @brief Fixed capacities reserved in every rendering snapshot slot.
     * @details All-zero (the default) disables automatic extraction; any non-zero field enables it.
     */
    FRenderSnapshotCapacities RenderCapacities{};
};

/**
 * @struct FWorldFrameContext
 * @brief Immutable context copied into each actor task.
 */
struct SCENE_EXPORT FWorldFrameContext
{
    UInt64 FrameIndex     = 0ULL;
    UInt64 FixedStepIndex = 0ULL;
    USize TaskCount       = 0ULL;
    EActorTickPhase Phase = EActorTickPhase::Update;
};

/**
 * @struct FWorldPhaseCompletion
 * @brief CPU-fence completion token for one dispatched phase.
 */
struct SCENE_EXPORT FWorldPhaseCompletion
{
    FWorldFrameContext Context;
    USize CompletedTaskCount = 0ULL;
    Bool bCapacityError      = false;
};

/**
 * @struct FWorldTickResult
 * @brief Observable work committed by one world tick.
 */
struct SCENE_EXPORT FWorldTickResult
{
    UInt64 FrameIndex         = 0ULL;
    UInt32 FixedStepsExecuted = 0U;
    USize DispatchedTaskCount = 0ULL;
    USize RoutedMessageCount  = 0ULL;
    FWorldPhaseCompletion LastPhase;
};

} // namespace Lumen
