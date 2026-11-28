/**
 * @file ReplicationSystem.hpp
 * @brief Deterministic, bounded authority/replica state machine over a byte transport.
 */

#pragma once

#include <SceneExport.hpp>

#include <Replication/ReplicationFrame.hpp>
#include <Replication/ReplicationTransport.hpp>
#include <Replication/ReplicationTypes.hpp>

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Map.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

/**
 * @class FReplicationSystem
 * @brief Runs on the game thread after the ingress journal and before hierarchy publication.
 * @details Every capacity is reserved by Initialize. Frames carry absolute values; dirty masks select fields.
 *          Only the Reliable channel is acked and baselined; Unreliable rejects old sequences without ack.
 *          Control frames are unsequenced and idempotent, so their loss cannot deadlock the protocol.
 */
class SCENE_EXPORT FReplicationSystem final : public FNonCopyable
{
public:

    FReplicationSystem () noexcept;
    ~FReplicationSystem () noexcept;

    /**
     * @brief Reserves all storage and fixes the injected collaborators.
     * @details Transport and handler are non-owning and must outlive this system.
     */
    [[nodiscard]] TExpected<void, EReplicationError> Initialize ( const FReplicationConfig &InConfig,
                                                                  EActorRole InRole,
                                                                  IReplicationTransport &InTransport,
                                                                  IReplicationActorHandler &InHandler ) noexcept;

    /** @return True after a successful Initialize. */
    [[nodiscard]] Bool IsInitialized () const noexcept;

    /** @return The configured authority role. */
    [[nodiscard]] EActorRole GetRole () const noexcept;

    /** @brief Allocates a persistent ID for an authority-side local actor. */
    [[nodiscard]] TExpected<void, EReplicationError> AssignPersistentId ( ActorID InLocalId ) noexcept;

    /** @brief Binds a replica-side local actor to an authority-allocated ID. */
    [[nodiscard]] TExpected<void, EReplicationError> BindPersistentId ( ActorID InLocalId,
                                                                        FPersistentActorId InPersistentId ) noexcept;

    /** @return The local ID bound to a persistent ID, if any. */
    [[nodiscard]] TOptional<ActorID> FindLocalId ( FPersistentActorId InId ) const noexcept;

    /** @return The persistent ID bound to a local actor, if any. */
    [[nodiscard]] TOptional<FPersistentActorId> FindPersistentId ( ActorID InLocalId ) const noexcept;

    /** @brief Sets the replicated visibility bit on the authority. */
    [[nodiscard]] TExpected<void, EReplicationError> SetVisibility ( FPersistentActorId InId, Bool bInVisible ) noexcept;

    /** @return The visibility bit of a known persistent ID; unknown IDs are visible. */
    [[nodiscard]] Bool GetVisibility ( FPersistentActorId InId ) const noexcept;

    /** @brief Builds/sends state or inputs, receives and applies frames, emits coalesced control records. */
    [[nodiscard]] TExpected<FReplicationTickResult, EReplicationError> Tick ( UInt64 InFrameIndex,
                                                                              const FReplicationSourceView &InSource ) noexcept;

    /** @return Latest-only observable counters. */
    [[nodiscard]] FReplicationStats GetStats () const noexcept;

    /** @return The last structured failure, if any. */
    [[nodiscard]] TOptional<EReplicationError> GetLastError () const noexcept;

    /**
     * @brief Copies received authority inputs into preallocated records for the caller's ingress journal.
     * @return Number of records copied; earlier records are consumed.
     */
    [[nodiscard]] USize CopyReceivedInputs ( TSpan<FReplicationInputRecord> OutRecords ) noexcept;

private:

    struct FActorState
    {
        Maths::FMat4x4f LocalTransform = Maths::FMat4x4f::Identity();
        FPersistentActorId Parent;
        Bool bVisible = true;
        Bool bAlive   = false;
    };

    struct FResendEntry
    {
        UInt64 Sequence = 0ULL;
        UInt64 FrameId  = 0ULL;
        UInt64 LastTick = 0ULL;
        USize Length    = 0ULL;
        UInt8 Attempts  = 0U;
    };

    struct FPendingParent
    {
        FPersistentActorId Child;
        FPersistentActorId Parent;
        UInt8 Ticks = 0U;
    };

    struct FGapEntry
    {
        UInt64 Sequence = 0ULL;
        USize Length    = 0ULL;
    };

    struct FWorkingActor
    {
        FPersistentActorId PersistentId;
        ActorID LocalId = InvalidActorID;
        FPersistentActorId ParentId;
        Maths::FMat4x4f LocalTransform = Maths::FMat4x4f::Identity();
        Bool bVisible                  = true;
    };

    void BuildWorkingView ( const FReplicationSourceView &InSource ) noexcept;
    TExpected<FReplicationTickResult, EReplicationError> TickAuthority ( UInt64 InFrameIndex,
                                                                         FReplicationTickResult &OutResult ) noexcept;
    TExpected<FReplicationTickResult, EReplicationError>
    TickReplica ( UInt64 InFrameIndex, const FReplicationSourceView &InSource, FReplicationTickResult &OutResult ) noexcept;
    TExpected<void, EReplicationError> SendControlFrame () noexcept;
    TExpected<void, EReplicationError> ProcessFrame ( const FReplicationFrame &InFrame,
                                                      FReplicationTickResult &OutResult ) noexcept;
    TExpected<void, EReplicationError> ProcessControlRecord ( const FReplicationRecord &InRecord ) noexcept;
    TExpected<void, EReplicationError> ApplyStateFrame ( const FReplicationFrame &InFrame,
                                                         FReplicationTickResult &OutResult ) noexcept;
    void AppendAbsoluteFields ( const FWorkingActor &InActor, FReplicationFrameBuilder &InOutBuilder ) noexcept;
    void QueueControl ( EReplicationControlKind InKind,
                        EReplicationDirection InDirection,
                        EReplicationChannel InChannel,
                        UInt64 InSequence ) noexcept;
    void
    UpdateCursorsFromAck ( EReplicationDirection InDirection, EReplicationChannel InChannel, UInt64 InAckedSequence ) noexcept;
    void TryResolvePendingParents () noexcept;
    [[nodiscard]] TOptional<FReplicationFrame> TryRecoverBufferedFrame ( UInt64 InExpectedSequence ) noexcept;
    void ApplyFrameToShadow ( const FReplicationFrame &InFrame, TMap<UInt64, FActorState> &InOutShadow ) noexcept;

private:

    Bool bInitialized                 = false;
    Bool bForceFullSnapshot           = false;
    EActorRole Role                   = EActorRole::Standalone;
    IReplicationTransport *Transport  = nullptr;
    IReplicationActorHandler *Handler = nullptr;

    FReplicationConfig Config;
    FReplicationStats Stats;
    TOptional<EReplicationError> LastError;

    UInt64 NextPersistentId            = 1ULL;
    UInt64 NextInputSequence           = 1ULL;
    UInt64 CurrentTick                 = 0ULL;
    UInt64 LastFullSnapshotTick        = 0ULL;
    UInt64 NextStateSequence           = 1ULL;
    USize ControlRecordsQueuedThisTick = 0ULL;

    FReplicationChannelStats Cursors[4];
    TMap<UInt64, ActorID> LocalByPersistent;
    TMap<ActorID, UInt64> PersistentByLocal;
    TMap<UInt64, FActorState> AckedShadow;
    TMap<UInt64, FActorState> AppliedShadow;
    TMap<UInt64, Bool> Visibility;

    TVector<FWorkingActor> WorkingView;
    TVector<UInt64> SortedIds;

    FReplicationFrameBuilder OutBuilder;
    FReplicationFrameBuilder InBuilder;
    TVector<Byte> OutPacket;
    TVector<Byte> InPacket;

    TVector<Byte> ResendBytes;
    TVector<FResendEntry> ResendEntries;

    TVector<Byte> GapBytes;
    TVector<FGapEntry> GapEntries;

    TVector<FReplicationRecord> PendingControl;
    TVector<FPendingParent> PendingParents;
    TVector<FReplicationInputRecord> ReceivedInputs;
};

} // namespace Lumen
