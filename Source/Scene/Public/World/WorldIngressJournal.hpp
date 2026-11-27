/**
 * @file WorldIngressJournal.hpp
 * @brief Bounded, sequenced journal of external world inputs used by deterministic replay.
 */

#pragma once

#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>
#include <SceneExport.hpp>

#include <Core/CoreTypes.hpp>
#include <HAL/PlatformMutex.hpp>

#include <Memory/Span.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

/**
 * @enum EWorldIngressKind
 * @brief The serial operation represented by one journal record.
 */
enum class EWorldIngressKind : UInt8
{
    Message,
    Broadcast,
    Destroy,
};

/**
 * @enum EWorldIngressError
 * @brief Structured failures accepted by journal append operations.
 */
enum class EWorldIngressError : UInt8
{
    CapacityExceeded,
    FrameOrderViolation,
};

/**
 * @struct FWorldIngressRecord
 * @brief Immutable external input tagged with its canonical ingress order and target frame.
 * @details `Sequence` is assigned once by the journal and is the only ordering promise.
 *          Producers run concurrently; the recorded order, not the observed thread
 *          scheduling, is what a replay reproduces. `TargetFrame` must be
 *          non-decreasing across appends so a world can consume the journal as a stream.
 */
struct SCENE_EXPORT FWorldIngressRecord
{
    UInt64 Sequence        = 0ULL;
    UInt64 TargetFrame     = 0ULL;
    EWorldIngressKind Kind = EWorldIngressKind::Message;
    ActorID Target         = InvalidActorID;
    ActorID Sender         = InvalidActorID;
    FMessage Message;
};

/** @class FWorldIngressJournal
 * @brief Fixed-capacity log of concurrent external inputs, consumed by FWorld during a tick.
 * @details Reserve once during a control operation. After reservation, appends never
 *          allocate: a full journal drops the record and reports `CapacityExceeded`
 *          without modifying accepted records. Appends are safe from several threads;
 *          reads take the same short lock so a consumer observes a coherent snapshot.
 *          A spawn is deliberately not a record type: replayable construction needs a
 *          type-erased factory, so spawns remain serial deferred commands of FWorld.
 */
class SCENE_EXPORT FWorldIngressJournal final
{
public:

    static constexpr USize DefaultCapacity = 256ULL;

    FWorldIngressJournal () noexcept;

    /** @brief Reserves the maximum number of retained records. Control operation. */
    void Reserve ( USize InCapacity );

    /** @brief Records a message addressed to one actor. */
    [[nodiscard]] TExpected<void, EWorldIngressError>
    AppendMessage ( UInt64 InTargetFrame, ActorID InTarget, ActorID InSender, const FMessage &InMessage ) noexcept;

    /** @brief Records a message addressed to every active actor. */
    [[nodiscard]] TExpected<void, EWorldIngressError>
    AppendBroadcast ( UInt64 InTargetFrame, ActorID InSender, const FMessage &InMessage ) noexcept;

    /** @brief Records a logical destroy request for one actor. */
    [[nodiscard]] TExpected<void, EWorldIngressError> AppendDestroy ( UInt64 InTargetFrame, ActorID InTarget ) noexcept;

    /** @brief Returns the number of retained records under the journal lock. */
    [[nodiscard]] USize GetRecordCount () const noexcept;

    /** @brief Returns the reserved record capacity. */
    [[nodiscard]] USize GetCapacity () const noexcept;

    /** @brief Returns the sequence that the next accepted record will receive. */
    [[nodiscard]] UInt64 GetNextSequence () const noexcept;

    /** @brief Returns the number of records rejected because the journal was full. */
    [[nodiscard]] UInt64 GetDroppedCount () const noexcept;

    /** @brief Returns the generation bumped by Clear, letting consumers reset their cursor. */
    [[nodiscard]] UInt64 GetGeneration () const noexcept;

    /** @brief Copies up to `OutRecords.size()` records in canonical sequence order. */
    [[nodiscard]] USize CopyRecords ( TSpan<FWorldIngressRecord> OutRecords ) const noexcept;

    /**
     * @brief Copies the ready records starting at one consumer index.
     * @details Records have non-decreasing target frames, so scanning can stop at the
     *          first future frame. A consumer keeps its own index and only copies the
     *          delta, avoiding an O(journal) copy on every tick.
     */
    [[nodiscard]] USize
    CopyReadyRecords ( UInt64 InMaxFrame, USize InStartIndex, TSpan<FWorldIngressRecord> OutRecords ) const noexcept;

    /** @brief Drops all records and restarts sequence numbering. */
    void Clear () noexcept;

private:

    [[nodiscard]] TExpected<void, EWorldIngressError> Append ( const FWorldIngressRecord &InRecord ) noexcept;

private:

    TVector<FWorldIngressRecord> Records;
    mutable HAL::FPlatformMutex Mutex;
    USize Capacity         = 0ULL;
    UInt64 NextSequence    = 1ULL;
    UInt64 LastTargetFrame = 0ULL;
    UInt64 DroppedCount    = 0ULL;
    UInt64 Generation      = 0ULL;
};

} // namespace Lumen
