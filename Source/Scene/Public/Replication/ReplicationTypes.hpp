/**
 * @file ReplicationTypes.hpp
 * @brief Bounded value types for deterministic state replication.
 */

#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorTypes.hpp>

#include <Core/CoreTypes.hpp>

#include <Maths/Matrix.hpp>

#include <Concepts/TriviallyCopyable.hpp>

#include <Memory/Span.hpp>

#include <Templates/Expected.hpp>

namespace Lumen
{

/** @brief Largest serialized state record; sizing of a replication frame uses this bound. */
inline constexpr SCENE_EXPORT USize MaxReplicationRecordBytes = 80ULL;

/** @brief Largest per-input payload carried inside a wire record. */
inline constexpr SCENE_EXPORT USize MaxReplicationInputPayloadBytes = 32ULL;

/** @brief Number of ranks reserved by the dirty-property mask. */
inline constexpr SCENE_EXPORT UInt8 MaxReplicationPropertyRanks = 32U;

/** @brief Worst-case records per actor in a full snapshot: spawn, transform, parent, visibility. */
inline constexpr SCENE_EXPORT USize MaxReplicationRecordsPerActor = 4ULL;

/**
 * @struct FPersistentActorId
 * @brief Process-independent actor identity; zero is invalid.
 * @details Never derived from ActorID slot/generation values. Only the authority allocates new values.
 */
struct SCENE_EXPORT FPersistentActorId final
{
    UInt64 Value = 0ULL;

    constexpr FPersistentActorId () noexcept = default;

    explicit constexpr FPersistentActorId ( const UInt64 InValue ) noexcept;

    /** @return True when the value is non-zero. */
    [[nodiscard]] constexpr Bool IsValid () const noexcept;

    /** @return The raw value, used only for canonical sorting and table keys. */
    [[nodiscard]] constexpr UInt64 GetValue () const noexcept;

    auto operator<=>( const FPersistentActorId & ) const noexcept = default;
};

inline constexpr SCENE_EXPORT FPersistentActorId InvalidPersistentActorId{};

static_assert( Concepts::CTriviallyCopyable<FPersistentActorId> );

/**
 * @enum EActorRole
 * @brief Replication authority semantics; Standalone keeps historical world behaviour.
 */
enum class EActorRole : UInt8
{
    Standalone,
    Authority,
    Replica,
};

/**
 * @enum EReplicationDirection
 * @brief Independent sequence space of a frame; acks always name their direction.
 */
enum class EReplicationDirection : UInt8
{
    StateToReplica,
    InputToAuthority,
};

/**
 * @enum EReplicationChannel
 * @brief Reliable is acked/baselined; Unreliable is fire-and-forget with old-sequence rejection.
 */
enum class EReplicationChannel : UInt8
{
    Reliable,
    Unreliable,
};

/**
 * @enum EActorReplicationKind
 * @brief Kind of one fixed-size state or control record.
 */
enum class EActorReplicationKind : UInt8
{
    Spawn      = 0U,
    Destroy    = 1U,
    Transform  = 2U,
    Parent     = 3U,
    Visibility = 4U,
    Input      = 5U,
    Control    = 6U,
    Count      = 7U,
};

/**
 * @enum EActorReplicationProperty
 * @brief Explicit ranks: the numeric value IS the canonical serialization order.
 * @details Alive must stay rank zero so a spawn precedes other fields of the same actor.
 *          Ranks 4..15 are reserved for later M9 extensions, 16..31 for world partition.
 */
enum class EActorReplicationProperty : UInt8
{
    Alive            = 0U,
    LocalTransform   = 1U,
    Parent           = 2U,
    Visibility       = 3U,
    Count            = 4U,
    M9ReservedFirst  = 4U,
    M9ReservedLast   = 15U,
    M10ReservedFirst = 16U,
    M10ReservedLast  = 31U,
};

static_assert( static_cast<UInt8>( EActorReplicationProperty::Alive ) == 0U );
static_assert( static_cast<UInt8>( EActorReplicationProperty::Count ) <= 16U );

/**
 * @enum EReplicationControlKind
 * @brief Idempotent control payload carried by unsequenced reliable control frames.
 */
enum class EReplicationControlKind : UInt8
{
    Ack             = 0U,
    Nack            = 1U,
    BaselineRequest = 2U,
    BaselineReady   = 3U,
};

/**
 * @struct FReplicationControlPayload
 * @brief Idempotent control payload of one record; never acked or resent itself.
 */
struct SCENE_EXPORT FReplicationControlPayload final
{
    EReplicationControlKind Kind    = EReplicationControlKind::Ack;
    EReplicationDirection Direction = EReplicationDirection::StateToReplica;
    EReplicationChannel Channel     = EReplicationChannel::Reliable;
    UInt64 Sequence                 = 0ULL;
};

static_assert( Concepts::CTriviallyCopyable<FReplicationControlPayload> );

/**
 * @enum EReplicationError
 * @brief Structured replication failures; no exception ever escapes a replication operation.
 */
enum class EReplicationError : UInt8
{
    None                = 0U,
    NotInitialized      = 1U,
    AlreadyInitialized  = 2U,
    InvalidCapacity     = 3U,
    InvalidPersistentId = 4U,
    AuthorityViolation  = 5U,
    CapacityExceeded    = 6U,
    TransportFailure    = 7U,
    MalformedFrame      = 8U,
    DuplicateSequence   = 9U,
    OutOfOrderSequence  = 10U,
    StaleBaseline       = 11U,
    UnknownActor        = 12U,
    ParentPending       = 13U,
    JournalFull         = 14U,
};

/**
 * @struct FDirtyPropertyMask
 * @brief Fixed 32-bit selection of which absolute fields a frame carries.
 */
struct SCENE_EXPORT FDirtyPropertyMask final
{
    UInt32 Bits = 0U;

    /** @brief Marks one property rank. */
    void Mark ( EActorReplicationProperty InProperty ) noexcept;

    /** @brief Clears one property rank. */
    void Unmark ( EActorReplicationProperty InProperty ) noexcept;

    /** @return Whether the rank is selected. */
    [[nodiscard]] Bool Has ( EActorReplicationProperty InProperty ) const noexcept;

    /** @brief Clears every bit. */
    void Clear () noexcept;

    /** @return Whether no rank is selected. */
    [[nodiscard]] Bool IsEmpty () const noexcept;

    /** @return The raw mask for serialization. */
    [[nodiscard]] UInt32 GetBits () const noexcept;
};

static_assert( Concepts::CTriviallyCopyable<FDirtyPropertyMask> );

/**
 * @struct FReplicationConfig
 * @brief Capacities reserved once by Initialize; all-zero MaxActors disables replication.
 */
struct SCENE_EXPORT FReplicationConfig final
{
    USize MaxActors                = 0ULL;
    USize MaxRecordsPerFrame       = 0ULL;
    USize MaxPayloadBytes          = 0ULL;
    USize ResendWindow             = 8ULL;
    USize MaxBufferedFrames        = 8ULL;
    USize MaxPendingControlRecords = 32ULL;
    USize MaxPendingParents        = 64ULL;
    USize MaxPendingInputs         = 256ULL;

    UInt8 MaxResendAttempts         = 2U;
    UInt8 FullSnapshotTickThreshold = 3U;
    UInt8 FullSnapshotCooldownTicks = 3U;
    UInt8 MaxPendingParentTicks     = 4U;

    /** @return InvalidCapacity when the worst-case full snapshot cannot fit MaxPayloadBytes. */
    [[nodiscard]] TExpected<void, EReplicationError> Validate () const noexcept;
};

/**
 * @struct FReplicationChannelStats
 * @brief Per-direction, per-channel observable counters.
 */
struct SCENE_EXPORT FReplicationChannelStats final
{
    UInt64 NextOutgoingSequence = 1ULL;
    UInt64 AckedSequence        = 0ULL;
    UInt64 LastSeenSequence     = 0ULL;
    UInt64 SentCount            = 0ULL;
    UInt64 ReceivedCount        = 0ULL;
    UInt64 DroppedCount         = 0ULL;
};

/**
 * @struct FReplicationStats
 * @brief Latest-only observable replication counters; never shared mutable state across threads.
 */
struct SCENE_EXPORT FReplicationStats final
{
    UInt64 FramesBuilt                      = 0ULL;
    UInt64 FramesApplied                    = 0ULL;
    UInt64 FramesDropped                    = 0ULL;
    UInt64 DuplicateFramesSuppressed        = 0ULL;
    UInt64 OutOfOrderRejected               = 0ULL;
    UInt64 StaleBaselineEscalations         = 0ULL;
    UInt64 FullSnapshotsSent                = 0ULL;
    UInt64 DroppedResendCount               = 0ULL;
    UInt64 OverflowFrameDropCount           = 0ULL;
    UInt64 PendingParentDrops               = 0ULL;
    UInt64 LastAppliedReliableSequence      = 0ULL;
    UInt64 LastAcknowledgedReliableSequence = 0ULL;
    FReplicationChannelStats Channels[2][2];
};

/**
 * @struct FReplicationActorSourceRecord
 * @brief Non-owning POD view of the committed hierarchy state needed by replication.
 */
struct SCENE_EXPORT FReplicationActorSourceRecord final
{
    ActorID LocalId                = InvalidActorID;
    ActorID ParentLocalId          = InvalidActorID;
    Maths::FMat4x4f LocalTransform = Maths::FMat4x4f::Identity();
};

static_assert( Concepts::CTriviallyCopyable<FReplicationActorSourceRecord> );

/**
 * @struct FReplicationSourceView
 * @brief Input of FReplicationSystem::Tick; the system canonicalizes it by persistent ID.
 */
struct SCENE_EXPORT FReplicationSourceView final
{
    TSpan<const FReplicationActorSourceRecord> Actors;
    UInt64 FrameIndex = 0ULL;
};

/**
 * @struct FReplicationInputRecord
 * @brief Bounded wire record produced on the replica and consumed on the authority.
 */
struct SCENE_EXPORT FReplicationInputRecord final
{
    FPersistentActorId Actor;
    UInt64 InputSequence                          = 0ULL;
    UInt16 PayloadSize                            = 0U;
    Byte Payload[MaxReplicationInputPayloadBytes] = {};
};

static_assert( Concepts::CTriviallyCopyable<FReplicationInputRecord> );

/**
 * @struct FReplicationTickResult
 * @brief Observable work of one replication tick.
 */
struct SCENE_EXPORT FReplicationTickResult final
{
    USize FramesSent         = 0ULL;
    USize FramesReceived     = 0ULL;
    USize RecordsApplied     = 0ULL;
    USize ControlRecordsSent = 0ULL;
    Bool bOverflow           = false;
};

/**
 * @class IReplicationActorHandler
 * @brief Injection boundary for actor lifecycle and hierarchy mutations on a replica.
 * @details Non-owning; the implementation is provided by composition or tests. No RHI and no concrete backend.
 */
class SCENE_EXPORT IReplicationActorHandler
{
public:

    virtual ~IReplicationActorHandler () noexcept = default;

    /** @brief Creates the local actor for one persistent identity. */
    [[nodiscard]] virtual TExpected<ActorID, EReplicationError> TrySpawn ( FPersistentActorId InId ) noexcept = 0;

    /** @brief Retires the local actor of one persistent identity. */
    [[nodiscard]] virtual TExpected<void, EReplicationError> TryDestroy ( ActorID InLocalId ) noexcept = 0;

    /** @brief Sets the absolute local transform before the serial hierarchy pass. */
    [[nodiscard]] virtual TExpected<void, EReplicationError>
    TrySetLocalTransform ( ActorID InLocalId, const Maths::FMat4x4f &InLocalTransform ) noexcept = 0;

    /** @brief Sets the weak parent by local ID, or InvalidActorID for a root. */
    [[nodiscard]] virtual TExpected<void, EReplicationError> TrySetParent ( ActorID InLocalId,
                                                                            ActorID InParentLocalId ) noexcept = 0;
};

/** @return Flat index of a direction/channel pair into FReplicationStats::Channels. */
[[nodiscard]] constexpr USize ReplicationChannelIndex ( const EReplicationDirection InDirection,
                                                        const EReplicationChannel InChannel ) noexcept;

} // namespace Lumen

#include "Inline/ReplicationTypes.inl"
