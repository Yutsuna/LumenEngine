/**
 * @file ReplicationBenchSupport.hpp
 * @brief Shared in-memory transport and actor handler for replication benchmarks.
 */

#pragma once

#include <Replication/ReplicationTransport.hpp>
#include <Replication/ReplicationTypes.hpp>

#include <Core/CoreTypes.hpp>

#include <Templates/Map.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Unexpected.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{
namespace BenchReplication
{

    [[nodiscard]] inline FReplicationConfig MakeConfig ( const USize InCount ) noexcept
    {
        FReplicationConfig Config;
        Config.MaxActors                 = InCount;
        Config.MaxRecordsPerFrame        = InCount * MaxReplicationRecordsPerActor;
        Config.MaxPayloadBytes           = ( InCount * MaxReplicationRecordsPerActor * MaxReplicationRecordBytes ) +
                                           ReplicationFrameHeaderBytes + ReplicationFrameChecksumBytes;
        Config.ResendWindow              = 8ULL;
        Config.MaxBufferedFrames         = 4ULL;
        Config.MaxPendingControlRecords  = 32ULL;
        Config.MaxPendingParents         = 16ULL;
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
            const USize Slot = Head;
            if ( Lengths[Slot] > OutBytes.size() )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::CapacityExceeded };
            }
            const USize Offset = Slot * PayloadBytes;
            for ( USize Index = 0ULL; Index < Lengths[Slot]; ++Index )
            {
                OutBytes[Index] = Payload[Offset + Index];
            }
            const USize Received = Lengths[Slot];
            Head                 = ( Head + 1ULL ) % Slots;
            --Count;
            return Received;
        }

    private:

        static constexpr USize Slots = 16ULL;
        USize PayloadBytes           = 0ULL;
        TVector<Byte> Payload;
        USize Lengths[Slots] = {};
        USize Head           = 0ULL;
        USize Count          = 0ULL;
    };

    class FNoopActorHandler final : public IReplicationActorHandler
    {
    public:

        [[nodiscard]] TExpected<ActorID, EReplicationError> TrySpawn ( const FPersistentActorId InId ) noexcept override
        {
            if ( NextLocal == 0ULL )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::CapacityExceeded };
            }
            static_cast<void>( InId );
            return NextLocal++;
        }

        [[nodiscard]] TExpected<void, EReplicationError> TryDestroy ( const ActorID InLocalId ) noexcept override
        {
            static_cast<void>( InLocalId );
            return {};
        }

        [[nodiscard]] TExpected<void, EReplicationError>
        TrySetLocalTransform ( const ActorID InLocalId, const Maths::FMat4x4f &InLocalTransform ) noexcept override
        {
            static_cast<void>( InLocalId );
            static_cast<void>( InLocalTransform );
            return {};
        }

        [[nodiscard]] TExpected<void, EReplicationError> TrySetParent ( const ActorID InLocalId,
                                                                        const ActorID InParentLocalId ) noexcept override
        {
            static_cast<void>( InLocalId );
            static_cast<void>( InParentLocalId );
            return {};
        }

    private:

        ActorID NextLocal = 1ULL;
    };

} // namespace BenchReplication
} // namespace Lumen
