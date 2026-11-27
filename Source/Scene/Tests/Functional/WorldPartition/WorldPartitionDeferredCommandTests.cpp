/**
 * @file WorldPartitionDeferredCommandTests.cpp
 * @brief Deferred partition commands, canonical journal replay and worker-independent digests.
 */

#include <World/World.hpp>
#include <World/WorldTypes.hpp>
#include <WorldPartition/WorldPartition.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorTypes.hpp>

#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    [[nodiscard]] FWorldDesc MakeDesc ( const UInt32 InWorkerCount ) noexcept
    {
        FWorldDesc Desc;
        Desc.WorkerCount       = InWorkerCount;
        Desc.MaxActors         = 16ULL;
        Desc.HierarchyCapacity = 16ULL;
        Desc.WorldId           = 99101ULL;
        Desc.PartitionConfig   = FWorldPartitionConfig{ .MaxCells = 8ULL, .MaxAssignedActors = 8ULL, .MaxJournalRecords = 32ULL };
        return Desc;
    }

    class FPartitionCommandActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FPartitionCommandActor );

    public:

        FPartitionCommandActor ( const ActorID InId, FWorld *InWorld, const FWorldCellId InCell ) noexcept
            : AActor( InId ), World( InWorld ), Cell( InCell )
        {
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            static_cast<void>( InMessage );
        }

        void OnTick ( const Float64 InDeltaTime ) noexcept override
        {
            static_cast<void>( InDeltaTime );
            if ( Issued.exchange( 1U, std::memory_order_acq_rel ) != 0U )
            {
                return;
            }
            const TExpected<void, EWorldPartitionError> Result = World->TryPartitionUnloadCell( Cell );
            if ( not Result )
            {
                LastError.store( static_cast<UInt32>( Result.Error() ), std::memory_order_release );
            }
        }

        [[nodiscard]] UInt32 GetLastError () const noexcept
        {
            return LastError.load( std::memory_order_acquire );
        }

    private:

        FWorld *World = nullptr;
        FWorldCellId Cell;
        TAtomic<UInt32> Issued    = { 0U };
        TAtomic<UInt32> LastError = { 0U };
    };

    [[nodiscard]] TSharedRef<AActor> SpawnIdleActor ( FWorld &InWorld )
    {
        class FIdleActor final : public AActor
        {
            LUMEN_ACTOR_BODY ( FIdleActor );

        public:

            explicit FIdleActor ( const ActorID InId ) noexcept : AActor( InId )
            {
            }

            void Receive ( const FMessage &InMessage ) noexcept override
            {
                static_cast<void>( InMessage );
            }
        };

        auto Spawned = InWorld.TrySpawnActor<FIdleActor>();
        LUMEN_CHECK_HAS_VALUE( Spawned );
        return *Spawned;
    }

    [[nodiscard]] UInt64 PartitionDigest ( const FWorld &InWorld ) noexcept
    {
        UInt64 Digest                    = 14695981039346656037ULL;
        const FWorldPartition *Partition = InWorld.GetPartition();
        if ( Partition == nullptr )
        {
            return Digest;
        }
        for ( const FPartitionJournalRecord &Record : Partition->GetJournal() )
        {
            const UInt64 Values[] = { static_cast<UInt64>( Record.Kind ), Record.CellId.GetValue(),
                                      Record.PersistentId.GetValue(), Record.FrameIndex, Record.SenderSequence };
            for ( const UInt64 Value : Values )
            {
                Digest ^= Value;
                Digest *= 1099511628211ULL;
            }
        }
        for ( UInt64 CellValue = 1ULL; CellValue <= 3ULL; ++CellValue )
        {
            const TOptional<EWorldCellState> State = Partition->FindCellState( FWorldCellId{ CellValue } );
            Digest ^= State ? static_cast<UInt64>( *State ) : 99ULL;
            Digest *= 1099511628211ULL;
        }
        return Digest;
    }

    [[nodiscard]] UInt64 BuildPartitionDigest ( const UInt32 InWorkerCount, const UInt64 InSeed )
    {
        FWorld World( MakeDesc( InWorkerCount ) );
        const FWorldCellId Target{ ( InSeed % 3ULL ) + 1ULL };
        for ( UInt64 CellValue = 1ULL; CellValue <= 3ULL; ++CellValue )
        {
            const TSharedRef<AActor> Member = SpawnIdleActor( World );
            LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Member->GetId(), FWorldCellId{ CellValue },
                                                               FPersistentActorId{ 600ULL + CellValue } ) );
            LUMEN_CHECK_HAS_VALUE( World.TryPartitionLoadCell( FWorldCellId{ CellValue } ) );
        }
        auto Commander = World.TrySpawnActor<FPartitionCommandActor>( &World, Target );
        LUMEN_CHECK_HAS_VALUE( Commander );
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        return PartitionDigest( World );
    }

} // namespace

LUMEN_TEST( FPartitionDeferredCommand, UnloadIsAppliedAtNextSerialCommit )
{
    FWorld World( MakeDesc( 2U ) );
    LUMEN_CHECK_TRUE( World.IsPartitionEnabled() );
    const FWorldCellId Cell{ 1ULL };
    const TSharedRef<AActor> Member = SpawnIdleActor( World );
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Member->GetId(), Cell, FPersistentActorId{ 500ULL } ) );
    LUMEN_CHECK_HAS_VALUE( World.TryPartitionLoadCell( Cell ) );
    LUMEN_CHECK_FALSE( World.IsActorDormant( Member->GetId() ) );

    auto Commander = World.TrySpawnActor<FPartitionCommandActor>( &World, Cell );
    LUMEN_CHECK_HAS_VALUE( Commander );

    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
    LUMEN_CHECK_EQ( ( *Commander )->GetLastError(), 0U );
    LUMEN_CHECK_FALSE( World.IsActorDormant( Member->GetId() ) );
    LUMEN_CHECK_EQ( World.GetPartition()->FindCellState( Cell ).ValueOr( EWorldCellState::Unloaded ), EWorldCellState::Loaded );

    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
    LUMEN_CHECK_TRUE( World.IsActorDormant( Member->GetId() ) );
    LUMEN_CHECK_EQ( World.GetPartition()->FindCellState( Cell ).ValueOr( EWorldCellState::Unloaded ), EWorldCellState::Unloaded );
}

LUMEN_TEST( FPartitionDeferredCommand, DuplicateCommandAndPhaseGuardAreStructured )
{
    FWorld World( MakeDesc( 1U ) );
    const FWorldCellId Cell{ 2ULL };
    const TSharedRef<AActor> Member = SpawnIdleActor( World );
    LUMEN_CHECK_HAS_VALUE( World.TryAssignActorToCell( Member->GetId(), Cell, FPersistentActorId{ 501ULL } ) );
    LUMEN_CHECK_HAS_VALUE( World.TryPartitionLoadCell( Cell ) );
    LUMEN_CHECK_HAS_ERROR( World.TryPartitionLoadCell( Cell ) );
    LUMEN_CHECK_HAS_VALUE( World.TryPartitionUnloadCell( Cell ) );
    LUMEN_CHECK_HAS_ERROR( World.TryPartitionUnloadCell( Cell ) );
    LUMEN_CHECK_HAS_ERROR( World.TryAssignActorToCell( InvalidActorID, Cell, FPersistentActorId{ 502ULL } ) );
}

LUMEN_TEST( FPartitionReplay, DigestIsStableAcrossSeedsRunsAndWorkers )
{
    constexpr TArray<UInt64, 3ULL> Seeds{ 17ULL, 0x5EEDULL, 987654321ULL };
    constexpr UInt32 Runs = 32U;
    constexpr TArray<UInt32, 3ULL> Workers{ 1U, 2U, 4U };

    for ( const UInt64 Seed : Seeds )
    {
        const UInt64 Expected = BuildPartitionDigest( 1U, Seed );
        for ( const UInt32 WorkerCount : Workers )
        {
            for ( UInt32 Run = 0U; Run < Runs; ++Run )
            {
                LUMEN_CHECK_EQ( BuildPartitionDigest( WorkerCount, Seed ), Expected );
            }
        }
    }
}

} // namespace Lumen
