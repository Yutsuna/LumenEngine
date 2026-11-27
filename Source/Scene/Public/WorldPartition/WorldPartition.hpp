/**
 * @file WorldPartition.hpp
 * @brief Per-world bounded logical cell registry with a replayable command journal.
 */

#pragma once

#include <SceneExport.hpp>

#include <WorldPartition/WorldPartitionTypes.hpp>

#include <Actor/ActorTypes.hpp>

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/Span.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Map.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

/**
 * @class FWorldPartition
 * @brief Owns cell membership and dormancy by weak ActorID; never owns actors and never allocates persistent IDs.
 * @details All storage is reserved by Initialize. Cell membership is control-time state; load/unload commands are
 *          applied serially by FWorld and journaled in canonical order. The persistent ID stored per actor is an
 *          opaque authority label used for ordering and conflict checks, never an allocation made here.
 */
class SCENE_EXPORT FWorldPartition final : public FNonCopyable, public FNonMovable
{
public:

    FWorldPartition () noexcept;
    ~FWorldPartition () noexcept;

    /** @brief Reserves every table once; a partially specified config is rejected. */
    [[nodiscard]] TExpected<void, EWorldPartitionError> Initialize ( const FWorldPartitionConfig &InConfig ) noexcept;

    /** @return True after a successful Initialize with a non-zero configuration. */
    [[nodiscard]] Bool IsInitialized () const noexcept;

    /** @brief Validates a prospective assignment without mutating state or appending to the journal. */
    [[nodiscard]] TExpected<void, EWorldPartitionError> CanAssignActor ( ActorID InActorId,
                                                                         FWorldCellId InCellId ) const noexcept;

    /** @brief Validates that an assigned actor can be unassigned; used before irreversible actor destruction. */
    [[nodiscard]] TExpected<void, EWorldPartitionError> CanUnassignActor ( ActorID InActorId ) const noexcept;

    /** @brief Registers one existing actor in a cell; control-time only. */
    [[nodiscard]] TExpected<void, EWorldPartitionError>
    TryAssignActor ( ActorID InActorId, FWorldCellId InCellId, FPersistentActorId InPersistentId ) noexcept;

    /** @brief Removes one actor from its cell; control-time only. */
    [[nodiscard]] TExpected<void, EWorldPartitionError> TryUnassignActor ( ActorID InActorId ) noexcept;

    /** @brief Commits a load after the last fence. */
    [[nodiscard]] TExpected<void, EWorldPartitionError>
    TryLoadCell ( FWorldCellId InCellId, UInt64 InFrameIndex, UInt64 InSenderSequence ) noexcept;

    /** @brief Commits an unload after the last fence. */
    [[nodiscard]] TExpected<void, EWorldPartitionError>
    TryUnloadCell ( FWorldCellId InCellId, UInt64 InFrameIndex, UInt64 InSenderSequence ) noexcept;

    /** @return The state of a known cell; empty for a cell that was never assigned an actor. */
    [[nodiscard]] TOptional<EWorldCellState> FindCellState ( FWorldCellId InCellId ) const noexcept;

    /** @return True for an actor assigned to a cell whose state is Unloaded. */
    [[nodiscard]] Bool IsActorDormant ( ActorID InActorId ) const noexcept;

    /** @return The cell of an assigned actor, if any. */
    [[nodiscard]] TOptional<FWorldCellId> FindActorCell ( ActorID InActorId ) const noexcept;

    /** @return The opaque persistent label supplied when the actor was assigned. */
    [[nodiscard]] TOptional<FPersistentActorId> FindActorPersistentId ( ActorID InActorId ) const noexcept;

    /** @return The actor currently carrying one persistent label, if any. */
    [[nodiscard]] TOptional<ActorID> FindActorByPersistentId ( FPersistentActorId InPersistentId ) const noexcept;

    /** @brief Copies actors of one cell into a caller-owned span; returns the count written. */
    [[nodiscard]] USize GetCellActors ( FWorldCellId InCellId, TSpan<ActorID> OutActors ) const noexcept;

    /** @return All committed mutations in canonical order; valid until the next mutation or Clear. */
    [[nodiscard]] TSpan<const FPartitionJournalRecord> GetJournal () const noexcept;

    /** @return Latest-only counters. */
    [[nodiscard]] FWorldPartitionStats GetStats () const noexcept;

    /** @brief Clears all state while retaining reserved storage. */
    void Clear () noexcept;

private:

    struct FCellEntry
    {
        FWorldCellId Id;
        EWorldCellState State = EWorldCellState::Unloaded;
        USize ActorCount      = 0ULL;
    };

    struct FActorEntry
    {
        ActorID Id = InvalidActorID;
        FWorldCellId CellId;
        FPersistentActorId PersistentId;
    };

    [[nodiscard]] Bool CanAppendJournal () const noexcept;
    [[nodiscard]] TExpected<void, EWorldPartitionError> AppendJournal ( EPartitionCommandKind InKind,
                                                                        FWorldCellId InCellId,
                                                                        ActorID InActorId,
                                                                        FPersistentActorId InPersistentId,
                                                                        UInt64 InFrameIndex,
                                                                        UInt64 InSenderSequence ) noexcept;

    TVector<FCellEntry> Cells;
    TVector<FActorEntry> Actors;
    TMap<UInt64, USize> CellIndex;
    TMap<ActorID, USize> ActorIndex;
    TMap<UInt64, ActorID> PersistentToActor;
    TVector<FPartitionJournalRecord> Journal;
    FWorldPartitionStats Stats;
    USize MaxCells          = 0ULL;
    USize MaxAssignedActors = 0ULL;
    USize MaxJournalRecords = 0ULL;
    Bool bInitialized       = false;
};

} // namespace Lumen
