/**
 * @file WorldIngressJournalTests.cpp
 * @brief Tests bounded external ingress recording and serial replay by FWorld.
 */

#include <World/World.hpp>
#include <World/WorldIngressJournal.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>

#include <Algo/Equal.hpp>
#include <Algo/Sort.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{

namespace
{

    /** @class FIngressCollector
     * @brief Records every delivered message in one single-consumer trace.
     */
    class FIngressCollector final : public AActor
    {
        LUMEN_ACTOR_BODY ( FIngressCollector );

    public:

        FIngressCollector ( const ActorID InId, TVector<UInt32> *InReceived ) noexcept : AActor( InId ), Received( InReceived )
        {
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            Received->push_back( InMessage.GetPayload<UInt32>() );
        }

    private:

        TVector<UInt32> *Received = nullptr;
    };

    /** @brief Appends one producer's deterministic slice from a worker thread. */
    void AppendProducerSlice ( FWorldIngressJournal &InJournal, const UInt32 InRank ) noexcept
    {
        constexpr UInt32 EntriesPerProducer = 100U;
        for ( UInt32 Index = 0U; Index < EntriesPerProducer; ++Index )
        {
            const UInt32 Payload = ( InRank * EntriesPerProducer ) + Index;
            static_cast<void>( InJournal.AppendMessage( 1ULL, 77ULL, static_cast<ActorID>( InRank ) + 1ULL,
                                                        FMessage::Make( 32U, static_cast<ActorID>( InRank ) + 1ULL, Payload ) ) );
        }
    }

} // namespace

LUMEN_TEST( FWorldIngressJournal, AppendsCanonicalSequenceAndRejectsOverflow )
{
    FWorldIngressJournal Journal;
    Journal.Reserve( 3ULL );

    LUMEN_CHECK_EQ( Journal.GetCapacity(), 3ULL );
    LUMEN_CHECK_EQ( Journal.GetRecordCount(), 0ULL );
    LUMEN_CHECK_EQ( Journal.GetNextSequence(), 1ULL );

    for ( UInt32 Index = 0U; Index < 3U; ++Index )
    {
        const TExpected<void, EWorldIngressError> Result =
            Journal.AppendMessage( 1ULL, 7ULL, 9ULL, FMessage::Make( 30U, 9ULL, Index ) );
        LUMEN_CHECK_HAS_VALUE( Result );
    }

    LUMEN_CHECK_EQ( Journal.GetRecordCount(), 3ULL );
    LUMEN_CHECK_EQ( Journal.GetNextSequence(), 4ULL );

    TVector<FWorldIngressRecord> Snapshot;
    Snapshot.resize( 8ULL );
    const USize Count = Journal.CopyRecords( TSpan<FWorldIngressRecord>( Snapshot.data(), Snapshot.size() ) );
    LUMEN_CHECK_EQ( Count, 3ULL );

    for ( USize Index = 0ULL; Index < Count; ++Index )
    {
        LUMEN_CHECK_EQ( Snapshot[Index].Sequence, Index + 1ULL );
        LUMEN_CHECK_EQ( Snapshot[Index].TargetFrame, 1ULL );
        LUMEN_CHECK_EQ( Snapshot[Index].Kind, EWorldIngressKind::Message );
        LUMEN_CHECK_EQ( Snapshot[Index].Target, 7ULL );
        LUMEN_CHECK_EQ( Snapshot[Index].Sender, 9ULL );
        LUMEN_CHECK_EQ( Snapshot[Index].Message.GetPayload<UInt32>(), static_cast<UInt32>( Index ) );
    }

    const TExpected<void, EWorldIngressError> Overflow =
        Journal.AppendMessage( 1ULL, 7ULL, 9ULL, FMessage::Make( 30U, 9ULL, 99U ) );
    LUMEN_CHECK_HAS_ERROR( Overflow );
    LUMEN_CHECK_EQ( Overflow.Error(), EWorldIngressError::CapacityExceeded );
    LUMEN_CHECK_EQ( Journal.GetDroppedCount(), 1ULL );
    LUMEN_CHECK_EQ( Journal.GetRecordCount(), 3ULL );
    LUMEN_CHECK_EQ( Journal.GetNextSequence(), 4ULL );
}

LUMEN_TEST( FWorldIngressJournal, RejectsOutOfOrderFramesAndClearRestartsGeneration )
{
    FWorldIngressJournal Journal;
    LUMEN_CHECK_HAS_VALUE( Journal.AppendBroadcast( 5ULL, 1ULL, FMessage::Make( 31U, 1ULL ) ) );

    const TExpected<void, EWorldIngressError> OutOfOrder = Journal.AppendBroadcast( 4ULL, 1ULL, FMessage::Make( 31U, 1ULL ) );
    LUMEN_CHECK_HAS_ERROR( OutOfOrder );
    LUMEN_CHECK_EQ( OutOfOrder.Error(), EWorldIngressError::FrameOrderViolation );
    LUMEN_CHECK_EQ( Journal.GetDroppedCount(), 0ULL );
    LUMEN_CHECK_EQ( Journal.GetRecordCount(), 1ULL );

    const UInt64 Generation = Journal.GetGeneration();
    Journal.Clear();
    LUMEN_CHECK_EQ( Journal.GetRecordCount(), 0ULL );
    LUMEN_CHECK_EQ( Journal.GetNextSequence(), 1ULL );
    LUMEN_CHECK_EQ( Journal.GetGeneration(), Generation + 1ULL );
}

LUMEN_TEST( FWorldIngressJournal, ConcurrentProducersReceiveUniqueCanonicalSequences )
{
    FWorldIngressJournal Journal;
    Journal.Reserve( 512ULL );

    FThread First( [&Journal] () noexcept
    {
        AppendProducerSlice( Journal, 0U );
    } );
    FThread Second( [&Journal] () noexcept
    {
        AppendProducerSlice( Journal, 1U );
    } );
    FThread Third( [&Journal] () noexcept
    {
        AppendProducerSlice( Journal, 2U );
    } );
    FThread Fourth( [&Journal] () noexcept
    {
        AppendProducerSlice( Journal, 3U );
    } );
    First.Join();
    Second.Join();
    Third.Join();
    Fourth.Join();

    constexpr USize ExpectedCount = 400ULL;
    LUMEN_CHECK_EQ( Journal.GetRecordCount(), ExpectedCount );
    LUMEN_CHECK_EQ( Journal.GetNextSequence(), ExpectedCount + 1ULL );
    LUMEN_CHECK_EQ( Journal.GetDroppedCount(), 0ULL );

    TVector<FWorldIngressRecord> Snapshot;
    Snapshot.resize( ExpectedCount );
    LUMEN_CHECK_EQ( Journal.CopyRecords( TSpan<FWorldIngressRecord>( Snapshot.data(), Snapshot.size() ) ), ExpectedCount );

    TVector<UInt32> Payloads;
    TVector<UInt32> Expected;
    Payloads.reserve( ExpectedCount );
    Expected.reserve( ExpectedCount );
    for ( USize Index = 0ULL; Index < ExpectedCount; ++Index )
    {
        LUMEN_CHECK_EQ( Snapshot[Index].Sequence, Index + 1ULL );
        Payloads.push_back( Snapshot[Index].Message.GetPayload<UInt32>() );
    }
    for ( UInt32 Rank = 0U; Rank < 4U; ++Rank )
    {
        for ( UInt32 Index = 0U; Index < 100U; ++Index )
        {
            Expected.push_back( ( Rank * 100U ) + Index );
        }
    }
    Algo::Sort( Payloads );
    Algo::Sort( Expected );
    LUMEN_CHECK_TRUE( Algo::Equal( Payloads, Expected ) );
}

LUMEN_TEST( FWorldIngressJournal, WorldAppliesMessagesBroadcastsAndDestroyByTargetFrame )
{
    FWorldIngressJournal Journal;
    Journal.Reserve( 4ULL );

    FWorldDesc Desc;
    Desc.WorkerCount = 1U;
    FWorld World( Desc );

    TVector<UInt32> PrimaryReceived;
    TVector<UInt32> SecondaryReceived;
    auto Primary   = World.SpawnActor<FIngressCollector>( &PrimaryReceived );
    auto Secondary = World.SpawnActor<FIngressCollector>( &SecondaryReceived );

    LUMEN_CHECK_HAS_VALUE( Journal.AppendMessage( 1ULL, Primary->GetId(), 0ULL, FMessage::Make( 30U, 0ULL, 11U ) ) );
    LUMEN_CHECK_HAS_VALUE( Journal.AppendBroadcast( 1ULL, 0ULL, FMessage::Make( 31U, 0ULL, 22U ) ) );
    LUMEN_CHECK_HAS_VALUE( Journal.AppendDestroy( 2ULL, Secondary->GetId() ) );
    World.SetIngressJournal( &Journal );

    const TExpected<FWorldTickResult, EWorldTickError> FirstTick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( FirstTick );
    LUMEN_CHECK_EQ( World.GetLastIngressRecordCount(), 2ULL );
    LUMEN_CHECK_EQ( PrimaryReceived.size(), 2ULL );
    LUMEN_CHECK_EQ( PrimaryReceived[0], 11U );
    LUMEN_CHECK_EQ( PrimaryReceived[1], 22U );
    LUMEN_CHECK_EQ( SecondaryReceived.size(), 1ULL );
    LUMEN_CHECK_EQ( SecondaryReceived[0], 22U );

    /** INFO: Records for a later frame stay pending until that frame is reached. */
    const TExpected<FWorldTickResult, EWorldTickError> SecondTick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( SecondTick );
    LUMEN_CHECK_EQ( World.GetLastIngressRecordCount(), 1ULL );
    LUMEN_CHECK_EQ( Secondary->GetLifecycle(), EActorLifecycle::Retired );
    const TExpected<void, EActorDestroyError> Repeated = World.TryDestroyActor( Secondary->GetId() );
    LUMEN_CHECK_HAS_ERROR( Repeated );
    LUMEN_CHECK_EQ( Repeated.Error(), EActorDestroyError::UnknownActor );

    /** INFO: Applied records are never delivered twice. */
    const TExpected<FWorldTickResult, EWorldTickError> ThirdTick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( ThirdTick );
    LUMEN_CHECK_EQ( World.GetLastIngressRecordCount(), 0ULL );
    LUMEN_CHECK_EQ( PrimaryReceived.size(), 2ULL );
    LUMEN_CHECK_EQ( SecondaryReceived.size(), 1ULL );
}

} // namespace Lumen
