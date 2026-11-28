/**
 * @file WorldPartitionTypes.hpp
 * @brief Bounded logical cell identity, state and journal contracts.
 */

#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorTypes.hpp>
#include <Replication/ReplicationTypes.hpp>

#include <Concepts/TriviallyCopyable.hpp>
#include <Core/CoreTypes.hpp>
#include <Templates/Expected.hpp>

namespace Lumen
{

/**
 * @struct FWorldCellId
 * @brief Strong process-local cell identity; zero is invalid.
 */
struct SCENE_EXPORT FWorldCellId final
{
    UInt64 Value = 0ULL;

    constexpr FWorldCellId () noexcept = default;

    explicit constexpr FWorldCellId ( const UInt64 InValue ) noexcept;

    /** @return True when the value is non-zero. */
    [[nodiscard]] constexpr Bool IsValid () const noexcept;

    /** @return The raw value, used only for canonical sorting and table keys. */
    [[nodiscard]] constexpr UInt64 GetValue () const noexcept;

    auto operator<=>( const FWorldCellId & ) const noexcept = default;
};

inline constexpr SCENE_EXPORT FWorldCellId InvalidWorldCellId{};

static_assert( Concepts::CTriviallyCopyable<FWorldCellId> );

/** @enum EWorldCellState @brief Logical streaming state of one cell. */
enum class EWorldCellState : UInt8
{
    Unloaded,
    Loaded,
};

/** @enum EPartitionCommandKind @brief Journaled partition mutations in canonical order. */
enum class EPartitionCommandKind : UInt8
{
    AssignActor   = 0U,
    UnassignActor = 1U,
    LoadCell      = 2U,
    UnloadCell    = 3U,
};

/** @enum EWorldPartitionError @brief Structured, non-throwing partition failures. */
enum class EWorldPartitionError : UInt8
{
    None                 = 0U,
    WorldClosed          = 1U,
    NotInitialized       = 2U,
    AlreadyInitialized   = 3U,
    InvalidCapacity      = 4U,
    InvalidCellId        = 5U,
    InvalidPersistentId  = 6U,
    UnknownCell          = 7U,
    UnknownActor         = 8U,
    ActorAlreadyAssigned = 9U,
    ActorNotAssigned     = 10U,
    DuplicateLoad        = 11U,
    CellNotLoaded        = 12U,
    PersistentConflict   = 13U,
    ControlPhaseRequired = 14U,
    CapacityExceeded     = 15U,
    JournalFull          = 16U,
    DeferredQueueFull    = 17U,
    ReplicationRejected  = 18U,
};

/**
 * @struct FWorldPartitionConfig
 * @brief Capacities reserved once at world construction; all-zero disables partitioning.
 * @details A partial configuration is invalid: every field must be zero or every field must be non-zero.
 */
struct SCENE_EXPORT FWorldPartitionConfig final
{
    USize MaxCells          = 0ULL;
    USize MaxAssignedActors = 0ULL;
    USize MaxJournalRecords = 0ULL;

    /** @return InvalidCapacity when the configuration is partially specified. */
    [[nodiscard]] TExpected<void, EWorldPartitionError> Validate () const noexcept;
};

/**
 * @struct FPartitionJournalRecord
 * @brief One replayable partition mutation with its canonical ordering key.
 */
struct SCENE_EXPORT FPartitionJournalRecord final
{
    UInt64 FrameIndex          = 0ULL;
    EPartitionCommandKind Kind = EPartitionCommandKind::LoadCell;
    FWorldCellId CellId;
    ActorID ActorId = InvalidActorID;
    FPersistentActorId PersistentId;
    UInt64 SenderSequence = 0ULL;
};

static_assert( Concepts::CTriviallyCopyable<FPartitionJournalRecord> );

/** @struct FWorldPartitionStats @brief Latest-only counters of one partition. */
struct SCENE_EXPORT FWorldPartitionStats final
{
    USize AssignedActorCount = 0ULL;
    USize LoadedCellCount    = 0ULL;
    USize JournalRecords     = 0ULL;
    UInt64 DroppedCommands   = 0ULL;
    UInt64 Conflicts         = 0ULL;
};

} // namespace Lumen

#include "Inline/WorldPartitionTypes.inl"
