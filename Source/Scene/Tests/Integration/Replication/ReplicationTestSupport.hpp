/**
 * @file ReplicationTestSupport.hpp
 * @brief In-memory transport and recording handler shared by replication integration tests.
 */

#pragma once

#include <Replication/ReplicationTransport.hpp>
#include <Replication/ReplicationTypes.hpp>

#include <Core/CoreTypes.hpp>

#include <Algo/Sort.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Map.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Unexpected.hpp>
#include <Templates/Vector.hpp>

#include <bit>

namespace Lumen
{
namespace ReplicationTests
{

    [[nodiscard]] inline FReplicationConfig MakeConfig ( const USize InCount ) noexcept
    {
        FReplicationConfig Config;
        Config.MaxActors          = InCount;
        Config.MaxRecordsPerFrame = InCount * MaxReplicationRecordsPerActor;
        Config.MaxPayloadBytes    = ( InCount * MaxReplicationRecordsPerActor * MaxReplicationRecordBytes ) +
                                 ReplicationFrameHeaderBytes + ReplicationFrameChecksumBytes;
        Config.ResendWindow              = 8ULL;
        Config.MaxBufferedFrames         = 4ULL;
        Config.MaxPendingControlRecords  = 32ULL;
        Config.MaxPendingParents         = 32ULL;
        Config.MaxPendingInputs          = 64ULL;
        Config.MaxResendAttempts         = 2U;
        Config.FullSnapshotTickThreshold = 3U;
        Config.FullSnapshotCooldownTicks = 3U;
        Config.MaxPendingParentTicks     = 4U;
        return Config;
    }

    class FMemoryTransport final : public IReplicationTransport
    {
    public:

        explicit FMemoryTransport ( const USize InPayloadBytes ) noexcept
            : PayloadBytes( InPayloadBytes ), Payload( Slots * InPayloadBytes )
        {
        }

        [[nodiscard]] TExpected<void, EReplicationError> TrySend ( TSpan<const Byte> InBytes ) noexcept override
        {
            if ( InBytes.size() > PayloadBytes or Count >= Slots )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::CapacityExceeded };
            }
            const USize Slot   = ( Head + Count ) % Slots;
            const USize Offset = Slot * PayloadBytes;
            for ( USize Index = 0ULL; Index < InBytes.size(); ++Index )
            {
                Payload[Offset + Index] = InBytes[Index];
            }
            Lengths[Slot] = InBytes.size();
            ++Count;
            return {};
        }

        [[nodiscard]] TExpected<USize, EReplicationError> TryReceive ( TSpan<Byte> OutBytes ) noexcept override
        {
            if ( Count == 0ULL )
            {
                return 0ULL;
            }
            if ( Lengths[Head] > OutBytes.size() )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::CapacityExceeded };
            }
            const USize Offset = Head * PayloadBytes;
            for ( USize Index = 0ULL; Index < Lengths[Head]; ++Index )
            {
                OutBytes[Index] = Payload[Offset + Index];
            }
            const USize Received = Lengths[Head];
            Head                 = ( Head + 1ULL ) % Slots;
            --Count;
            return Received;
        }

        [[nodiscard]] USize GetQueuedCount () const noexcept
        {
            return Count;
        }

        /** @brief Drops the oldest queued frame; used to simulate transport loss. */
        Bool TryDropOldest () noexcept
        {
            if ( Count == 0ULL )
            {
                return false;
            }
            Head = ( Head + 1ULL ) % Slots;
            --Count;
            return true;
        }

    private:

        static constexpr USize Slots = 16ULL;
        USize PayloadBytes           = 0ULL;
        TVector<Byte> Payload;
        USize Lengths[Slots] = {};
        USize Head           = 0ULL;
        USize Count          = 0ULL;
    };

    /** @brief Records every applied value keyed by persistent ID so tests can compare canonical state. */
    class FRecordingHandler final : public IReplicationActorHandler
    {
    public:

        struct FRecordedState
        {
            ActorID LocalId           = InvalidActorID;
            Maths::FMat4x4f Transform = Maths::FMat4x4f::Identity();
            ActorID Parent            = InvalidActorID;
            Bool bVisible             = true;
        };

        [[nodiscard]] TExpected<ActorID, EReplicationError> TrySpawn ( const FPersistentActorId InId ) noexcept override
        {
            const UInt64 Value = InId.GetValue();
            if ( States.find( Value ) != States.end() )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::InvalidPersistentId };
            }
            FRecordedState State;
            State.LocalId = NextLocal++;
            States.emplace( Value, State );
            return State.LocalId;
        }

        [[nodiscard]] TExpected<void, EReplicationError> TryDestroy ( const ActorID InLocalId ) noexcept override
        {
            for ( auto It = States.begin(); It != States.end(); ++It )
            {
                if ( It->second.LocalId == InLocalId )
                {
                    States.erase( It );
                    return {};
                }
            }
            return TUnexpected<EReplicationError>{ EReplicationError::UnknownActor };
        }

        [[nodiscard]] TExpected<void, EReplicationError>
        TrySetLocalTransform ( const ActorID InLocalId, const Maths::FMat4x4f &InTransform ) noexcept override
        {
            FRecordedState *State = FindByLocal( InLocalId );
            if ( State == nullptr )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::UnknownActor };
            }
            State->Transform = InTransform;
            return {};
        }

        [[nodiscard]] TExpected<void, EReplicationError> TrySetParent ( const ActorID InLocalId,
                                                                        const ActorID InParentLocalId ) noexcept override
        {
            FRecordedState *State = FindByLocal( InLocalId );
            if ( State == nullptr )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::UnknownActor };
            }
            State->Parent = InParentLocalId;
            return {};
        }

        [[nodiscard]] const TMap<UInt64, FRecordedState> &GetStates () const noexcept
        {
            return States;
        }

        /** @brief FNV-1a digest over canonical state values; local IDs and sequences are excluded. */
        [[nodiscard]] UInt64 GetDigest () const noexcept
        {
            TVector<UInt64> Keys;
            Keys.reserve( States.size() );
            for ( const auto &Entry : States )
            {
                Keys.push_back( Entry.first );
            }
            Algo::Sort( Keys );
            UInt64 Digest = 0xCBF29CE484222325ULL;
            for ( const UInt64 Key : Keys )
            {
                Digest                      = Fold( Digest, Key );
                const FRecordedState &State = States.at( Key );
                for ( USize Column = 0ULL; Column < 4ULL; ++Column )
                {
                    for ( USize Row = 0ULL; Row < 4ULL; ++Row )
                    {
                        const Float32 Value     = State.Transform[Column].Data[Row];
                        const Float32 Canonical = Value == 0.0F ? 0.0F : Value;
                        const UInt32 Bits       = std::bit_cast<UInt32>( Canonical );
                        Digest                  = Fold( Digest, static_cast<UInt64>( Bits ) );
                    }
                }
                Digest = Fold( Digest, State.Parent == InvalidActorID ? 0ULL : 1ULL );
                Digest = Fold( Digest, State.bVisible ? 1ULL : 0ULL );
            }
            return Digest;
        }

    private:

        [[nodiscard]] FRecordedState *FindByLocal ( const ActorID InLocalId ) noexcept
        {
            for ( auto &Entry : States )
            {
                if ( Entry.second.LocalId == InLocalId )
                {
                    return &Entry.second;
                }
            }
            return nullptr;
        }

        [[nodiscard]] static UInt64 Fold ( UInt64 InDigest, const UInt64 InValue ) noexcept
        {
            InDigest ^= InValue;
            return InDigest * 0x100000001B3ULL;
        }

        TMap<UInt64, FRecordedState> States;
        ActorID NextLocal = 1ULL;
    };

} // namespace ReplicationTests
} // namespace Lumen
