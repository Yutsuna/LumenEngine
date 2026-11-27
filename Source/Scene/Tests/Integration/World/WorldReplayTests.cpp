/**
 * @file WorldReplayTests.cpp
 * @brief Replays the same canonical emission journal with 1, 2 and 4 workers.
 */

#include <World/World.hpp>
#include <World/WorldTypes.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

#include <Algo/Sort.hpp>
#include <Templates/Vector.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    /** @class FEmittingActor
     * @brief Emits a deterministic burst tagged with its own ID.
     */
    class FEmittingActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FEmittingActor );

    public:

        FEmittingActor ( const ActorID InId, const ActorID InTarget, const UInt32 InBurst ) noexcept
            : AActor( InId ), Target( InTarget ), Burst( InBurst )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
        }

        void OnTick ( const Float64 ) noexcept override
        {
            for ( UInt32 Index = 0U; Index < Burst; ++Index )
            {
                TExpected<void, EActorSendError> Result =
                    TrySendMessage( Target, FMessage::Make( 73U, GetId(), static_cast<UInt32>( GetId() ) ) );
                if ( Result )
                {
                    EmittedCount.fetch_add( 1U, std::memory_order_relaxed );
                }
                else
                {
                    LastError.store( static_cast<UInt32>( Result.Error() ), std::memory_order_release );
                }
            }
        }

        [[nodiscard]] UInt32 GetEmittedCount () const noexcept
        {
            return EmittedCount.load( std::memory_order_relaxed );
        }

        [[nodiscard]] UInt32 GetLastError () const noexcept
        {
            return LastError.load( std::memory_order_relaxed );
        }

    private:

        ActorID Target = InvalidActorID;
        UInt32 Burst   = 0U;

        TAtomic<UInt32> EmittedCount = { 0U };
        TAtomic<UInt32> LastError    = { 0U };
    };

    /** @class FCollectingActor
     * @brief Appends received payloads to a single-consumer trace.
     */
    class FCollectingActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FCollectingActor );

    public:

        FCollectingActor ( const ActorID InId, TVector<UInt32> *InTrace ) noexcept : AActor( InId ), Trace( InTrace )
        {
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            Trace->push_back( InMessage.GetPayload<UInt32>() );
        }

    private:

        TVector<UInt32> *Trace = nullptr;
    };

    /** @class FJournalSenderActor
     * @brief Emits one seeded payload per tick derived only from local state.
     */
    class FJournalSenderActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FJournalSenderActor );

    public:

        FJournalSenderActor ( const ActorID InId, const ActorID InTarget, const UInt32 InSeed ) noexcept
            : AActor( InId ), Target( InTarget ), Seed( InSeed )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
        }

        void OnTick ( const Float64 ) noexcept override
        {
            const UInt32 Payload                          = Seed ^ ( Sequence << 8U );
            const TExpected<void, EActorSendError> Result = TrySendMessage( Target, FMessage::Make( 90U, GetId(), Payload ) );
            if ( Result )
            {
                ++Sequence;
            }
            else
            {
                LastError.store( static_cast<UInt32>( Result.Error() ) + 1U, std::memory_order_release );
            }
        }

        [[nodiscard]] UInt32 GetLastError () const noexcept
        {
            return LastError.load( std::memory_order_acquire );
        }

    private:

        ActorID Target  = InvalidActorID;
        UInt32 Seed     = 0U;
        UInt32 Sequence = 0U;

        TAtomic<UInt32> LastError = { 0U };
    };

    /** @class FJournalCommandActor
     * @brief Requests a deferred broadcast on a fixed seeded cadence.
     */
    class FJournalCommandActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FJournalCommandActor );

    public:

        FJournalCommandActor ( const ActorID InId, FWorld *InWorld, const UInt32 InSeed ) noexcept
            : AActor( InId ), World( InWorld ), Seed( InSeed )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
        }

        void OnTick ( const Float64 ) noexcept override
        {
            if ( TickIndex % 5U == 0U )
            {
                const UInt32 Payload                          = 0x4000'0000U ^ Seed ^ ( TickIndex << 8U );
                const TExpected<void, EActorSendError> Result = World->TryBroadcast( FMessage::Make( 91U, GetId(), Payload ) );
                if ( not Result )
                {
                    LastError.store( static_cast<UInt32>( Result.Error() ) + 1U, std::memory_order_release );
                }
            }
            ++TickIndex;
        }

        [[nodiscard]] UInt32 GetLastError () const noexcept
        {
            return LastError.load( std::memory_order_acquire );
        }

    private:

        FWorld *World    = nullptr;
        UInt32 Seed      = 0U;
        UInt32 TickIndex = 0U;

        TAtomic<UInt32> LastError = { 0U };
    };

    /** @class FJournalHashingActor
     * @brief Folds every received payload into one canonical FNV-1a digest.
     */
    class FJournalHashingActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FJournalHashingActor );

    public:

        explicit FJournalHashingActor ( const ActorID InId, TAtomic<UInt64> *InDigest ) noexcept
            : AActor( InId ), Digest( InDigest )
        {
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            UInt64 Value = Digest->load( std::memory_order_relaxed );
            Value ^= static_cast<UInt64>( InMessage.GetPayload<UInt32>() );
            Value *= 1099511628211ULL;
            Digest->store( Value, std::memory_order_relaxed );
        }

    private:

        TAtomic<UInt64> *Digest = nullptr;
    };

} // namespace

LUMEN_INTEGRATION_TEST( FWorld_Replay, CanonicalTraceMatchesAcrossWorkerCounts )
{
    constexpr UInt32 WorkerCounts[] = { 1U, 2U, 4U };
    constexpr UInt32 Burst          = 3U;

    TVector<UInt32> CanonicalRanks;

    for ( const UInt32 WorkerCount : WorkerCounts )
    {
        FWorldDesc Desc;
        Desc.WorkerCount = WorkerCount;

        FWorld World( Desc );
        TVector<UInt32> Received;

        auto Receiver = World.SpawnActor<FCollectingActor>( &Received );
        auto First    = World.SpawnActor<FEmittingActor>( Receiver->GetId(), Burst );
        auto Second   = World.SpawnActor<FEmittingActor>( Receiver->GetId(), Burst );
        auto Third    = World.SpawnActor<FEmittingActor>( Receiver->GetId(), Burst );

        const TExpected<FWorldTickResult, EWorldTickError> EmitTick = World.TryTick( 0.0 );
        LUMEN_CHECK_HAS_VALUE( EmitTick );
        LUMEN_CHECK_EQ( EmitTick->RoutedMessageCount, 3ULL * static_cast<UInt64>( Burst ) );
        LUMEN_CHECK_EQ( Received.size(), 0ULL );
        LUMEN_CHECK_EQ( First->GetEmittedCount(), Burst );
        LUMEN_CHECK_EQ( Second->GetEmittedCount(), Burst );
        LUMEN_CHECK_EQ( Third->GetEmittedCount(), Burst );
        LUMEN_CHECK_EQ( First->GetLastError(), 0U );
        LUMEN_CHECK_EQ( Second->GetLastError(), 0U );
        LUMEN_CHECK_EQ( Third->GetLastError(), 0U );

        const TExpected<FWorldTickResult, EWorldTickError> ConsumeTick = World.TryTick( 0.0 );
        LUMEN_CHECK_HAS_VALUE( ConsumeTick );
        LUMEN_CHECK_EQ( Received.size(), 3ULL * static_cast<UInt64>( Burst ) );

        /** INFO: Merge sorts by (Target, Sender, SenderSequence), never by hash-map or worker order. */
        TVector<ActorID> SenderIds;
        SenderIds.push_back( First->GetId() );
        SenderIds.push_back( Second->GetId() );
        SenderIds.push_back( Third->GetId() );
        Algo::Sort( SenderIds, [] ( const ActorID InLeft, const ActorID InRight ) noexcept -> Bool
        {
            return InLeft < InRight;
        } );

        for ( USize SenderIndex = 0ULL; SenderIndex < SenderIds.size(); ++SenderIndex )
        {
            for ( UInt32 Sequence = 0U; Sequence < Burst; ++Sequence )
            {
                const UInt32 Expected  = static_cast<UInt32>( SenderIds[SenderIndex] );
                const USize TraceIndex = SenderIndex * static_cast<USize>( Burst ) + static_cast<USize>( Sequence );
                LUMEN_CHECK_EQ( Received[TraceIndex], Expected );
            }
        }

        /** INFO: Normalize absolute IDs to sender ranks so distinct worlds can be compared. */
        TVector<UInt32> Ranks;
        for ( const UInt32 Payload : Received )
        {
            USize Rank = 0ULL;
            while ( Rank < SenderIds.size() and static_cast<UInt32>( SenderIds[Rank] ) != Payload )
            {
                ++Rank;
            }
            LUMEN_CHECK_TRUE( Rank < SenderIds.size() );
            Ranks.push_back( static_cast<UInt32>( Rank ) );
        }

        if ( CanonicalRanks.empty() )
        {
            CanonicalRanks = Ranks;
        }
        else
        {
            LUMEN_CHECK_EQ( Ranks.size(), CanonicalRanks.size() );
            for ( USize Index = 0ULL; Index < CanonicalRanks.size(); ++Index )
            {
                LUMEN_CHECK_EQ( Ranks[Index], CanonicalRanks[Index] );
            }
        }
    }
}

LUMEN_INTEGRATION_TEST( FWorld_Replay, CanonicalDigestIsSeedStableAcrossRunsAndWorkers )
{
    constexpr UInt64 Seeds[]        = { 17ULL, 0x5EEDULL, 987654321ULL };
    constexpr UInt32 WorkerCounts[] = { 1U, 2U, 4U };
    constexpr UInt32 Runs           = 32U;
    constexpr UInt32 TickCount      = 100U;
    constexpr UInt32 SenderCount    = 4U;

    for ( const UInt64 Seed : Seeds )
    {
        UInt64 CanonicalDigest     = 0ULL;
        UInt64 CanonicalDispatched = 0ULL;
        UInt64 CanonicalRouted     = 0ULL;
        Bool bHasCanonical         = false;

        for ( const UInt32 WorkerCount : WorkerCounts )
        {
            for ( UInt32 RunIndex = 0U; RunIndex < Runs; ++RunIndex )
            {
                FWorldDesc Desc;
                Desc.WorkerCount = WorkerCount;
                Desc.MaxActors   = SenderCount + 2ULL;

                FWorld World( Desc );
                TAtomic<UInt64> Digest = { 0ULL };

                auto Collector = World.SpawnActor<FJournalHashingActor>( &Digest );
                for ( UInt32 SenderIndex = 0U; SenderIndex < SenderCount; ++SenderIndex )
                {
                    auto Sender = World.SpawnActor<FJournalSenderActor>( Collector->GetId(), static_cast<UInt32>( Seed ) );
                    LUMEN_CHECK_TRUE( Sender->GetLastError() == 0U );
                }
                auto Commander = World.SpawnActor<FJournalCommandActor>( &World, static_cast<UInt32>( Seed ) );

                UInt64 DispatchedTotal = 0ULL;
                UInt64 RoutedTotal     = 0ULL;
                for ( UInt32 TickIndex = 0U; TickIndex < TickCount; ++TickIndex )
                {
                    const TExpected<FWorldTickResult, EWorldTickError> Result = World.TryTick( 0.016 );
                    LUMEN_CHECK_HAS_VALUE( Result );
                    DispatchedTotal += Result->DispatchedTaskCount;
                    RoutedTotal += Result->RoutedMessageCount;
                }

                LUMEN_CHECK_EQ( Commander->GetLastError(), 0U );

                const UInt64 ObservedDigest = Digest.load( std::memory_order_acquire );
                if ( not bHasCanonical )
                {
                    CanonicalDigest     = ObservedDigest;
                    CanonicalDispatched = DispatchedTotal;
                    CanonicalRouted     = RoutedTotal;
                    bHasCanonical       = true;
                }
                else
                {
                    LUMEN_CHECK_EQ( ObservedDigest, CanonicalDigest );
                    LUMEN_CHECK_EQ( DispatchedTotal, CanonicalDispatched );
                    LUMEN_CHECK_EQ( RoutedTotal, CanonicalRouted );
                }
            }
        }

        LUMEN_CHECK_TRUE( bHasCanonical );
        LUMEN_CHECK_NOT_EQ( CanonicalDigest, 0ULL );
    }
}

} // namespace Lumen
