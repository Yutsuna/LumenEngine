/**
 * @file WorldIngressReplayTests.cpp
 * @brief Replays one external ingress journal with 1, 2 and 4 world workers.
 */

#include <World/World.hpp>
#include <World/WorldIngressJournal.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    /** @class FReplayHashActor
     * @brief Folds each received ingress payload into one canonical FNV-1a digest.
     */
    class FReplayHashActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FReplayHashActor );

    public:

        explicit FReplayHashActor ( const ActorID InId, TAtomic<UInt64> *InDigest ) noexcept : AActor( InId ), Digest( InDigest )
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

    /** @brief Builds a deterministic external ingress journal for one seed. */
    void FillJournal ( FWorldIngressJournal &InJournal, const UInt32 InSeed )
    {
        constexpr UInt32 TickCount = 100U;
        for ( UInt32 Frame = 1U; Frame <= TickCount; ++Frame )
        {
            const UInt64 TargetFrame = static_cast<UInt64>( Frame );
            const UInt32 First       = InSeed ^ ( Frame << 8U );
            const UInt32 Second      = 0x7F00'0000U ^ InSeed ^ ( Frame << 8U );
            LUMEN_CHECK_HAS_VALUE( InJournal.AppendBroadcast( TargetFrame, 0ULL, FMessage::Make( 40U, 0ULL, First ) ) );
            LUMEN_CHECK_HAS_VALUE( InJournal.AppendBroadcast( TargetFrame, 0ULL, FMessage::Make( 41U, 0ULL, Second ) ) );
        }
    }

} // namespace

LUMEN_INTEGRATION_TEST( FWorldIngressReplay, CanonicalDigestIsJournalStableAcrossRunsAndWorkers )
{
    constexpr UInt64 Seeds[]        = { 17ULL, 0x5EEDULL, 987654321ULL };
    constexpr UInt32 WorkerCounts[] = { 1U, 2U, 4U };
    constexpr UInt32 Runs           = 32U;
    constexpr UInt32 TickCount      = 100U;
    constexpr UInt32 RecordCount    = TickCount * 2U;

    for ( const UInt64 Seed : Seeds )
    {
        FWorldIngressJournal Journal;
        Journal.Reserve( RecordCount );
        FillJournal( Journal, static_cast<UInt32>( Seed ) );
        LUMEN_CHECK_EQ( Journal.GetRecordCount(), static_cast<USize>( RecordCount ) );

        UInt64 CanonicalFirst  = 0ULL;
        UInt64 CanonicalSecond = 0ULL;
        Bool bHasCanonical     = false;

        for ( const UInt32 WorkerCount : WorkerCounts )
        {
            for ( UInt32 RunIndex = 0U; RunIndex < Runs; ++RunIndex )
            {
                FWorldDesc Desc;
                Desc.WorkerCount = WorkerCount;
                Desc.MaxActors   = 3ULL;

                FWorld World( Desc );
                TAtomic<UInt64> FirstDigest  = { 0ULL };
                TAtomic<UInt64> SecondDigest = { 0ULL };

                auto First  = World.SpawnActor<FReplayHashActor>( &FirstDigest );
                auto Second = World.SpawnActor<FReplayHashActor>( &SecondDigest );
                LUMEN_CHECK_TRUE( First->GetId() != Second->GetId() );
                World.SetIngressJournal( &Journal );

                UInt64 AppliedTotal = 0ULL;
                for ( UInt32 TickIndex = 0U; TickIndex < TickCount; ++TickIndex )
                {
                    const TExpected<FWorldTickResult, EWorldTickError> Result = World.TryTick( 0.0 );
                    LUMEN_CHECK_HAS_VALUE( Result );
                    LUMEN_CHECK_EQ( World.GetLastIngressRecordCount(), 2ULL );
                    AppliedTotal += World.GetLastIngressRecordCount();
                }
                LUMEN_CHECK_EQ( AppliedTotal, static_cast<UInt64>( RecordCount ) );

                const UInt64 ObservedFirst  = FirstDigest.load( std::memory_order_acquire );
                const UInt64 ObservedSecond = SecondDigest.load( std::memory_order_acquire );
                if ( not bHasCanonical )
                {
                    CanonicalFirst  = ObservedFirst;
                    CanonicalSecond = ObservedSecond;
                    bHasCanonical   = true;
                }
                else
                {
                    LUMEN_CHECK_EQ( ObservedFirst, CanonicalFirst );
                    LUMEN_CHECK_EQ( ObservedSecond, CanonicalSecond );
                }
            }
        }

        LUMEN_CHECK_TRUE( bHasCanonical );
        LUMEN_CHECK_NOT_EQ( CanonicalFirst, 0ULL );
        LUMEN_CHECK_NOT_EQ( CanonicalSecond, 0ULL );
    }
}

} // namespace Lumen
