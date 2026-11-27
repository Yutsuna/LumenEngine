/**
 * @file WorldPartitionTests.cpp
 * @brief Bounded cell membership, capacity, conflict and journal contracts.
 */

#include <WorldPartition/WorldPartition.hpp>
#include <WorldPartition/WorldPartitionTypes.hpp>

#include <Actor/ActorTypes.hpp>

#include <AllocationTracking.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    [[nodiscard]] FWorldPartitionConfig
    MakeConfig ( const USize InCells = 4ULL, const USize InActors = 8ULL, const USize InJournal = 16ULL ) noexcept
    {
        return FWorldPartitionConfig{ .MaxCells = InCells, .MaxAssignedActors = InActors, .MaxJournalRecords = InJournal };
    }

    [[nodiscard]] ActorID ProbeActor ( const USize InIndex ) noexcept
    {
        return MakeActorId( static_cast<UInt32>( InIndex ) + 100U, 5U );
    }

    [[nodiscard]] FPersistentActorId ProbePersistent ( const USize InIndex ) noexcept
    {
        return FPersistentActorId{ 1000ULL + static_cast<UInt64>( InIndex ) };
    }

} // namespace

LUMEN_TEST( FWorldPartitionConfig, PartialConfigurationIsRejected )
{
    const FWorldPartitionConfig Disabled;
    LUMEN_CHECK_HAS_VALUE( Disabled.Validate() );
    const FWorldPartitionConfig Partial{ .MaxCells = 2ULL, .MaxAssignedActors = 0ULL, .MaxJournalRecords = 2ULL };
    LUMEN_CHECK_HAS_ERROR( Partial.Validate() );
    LUMEN_CHECK_HAS_VALUE( MakeConfig().Validate() );
}

LUMEN_TEST( FWorldPartition, UnknownAndInvalidInputsAreRejected )
{
    FWorldPartition Partition;
    LUMEN_CHECK_FALSE( Partition.IsInitialized() );
    LUMEN_CHECK_HAS_ERROR( Partition.TryAssignActor( ProbeActor( 0ULL ), InvalidWorldCellId, ProbePersistent( 0ULL ) ) );
    LUMEN_CHECK_HAS_VALUE( Partition.Initialize( MakeConfig() ) );
    LUMEN_CHECK_TRUE( Partition.IsInitialized() );
    LUMEN_CHECK_HAS_ERROR( Partition.Initialize( MakeConfig() ) );
    LUMEN_CHECK_HAS_ERROR( Partition.TryAssignActor( ProbeActor( 0ULL ), InvalidWorldCellId, ProbePersistent( 0ULL ) ) );
    LUMEN_CHECK_HAS_ERROR( Partition.TryAssignActor( ProbeActor( 0ULL ), FWorldCellId{ 1ULL }, InvalidPersistentActorId ) );
    LUMEN_CHECK_HAS_ERROR( Partition.TryLoadCell( FWorldCellId{ 99ULL }, 0ULL, 0ULL ) );
    LUMEN_CHECK_HAS_ERROR( Partition.TryUnloadCell( FWorldCellId{ 99ULL }, 0ULL, 0ULL ) );
    LUMEN_CHECK_FALSE( Partition.IsActorDormant( ProbeActor( 0ULL ) ) );
}

LUMEN_TEST( FWorldPartition, AssignmentLoadUnloadAndJournalAreCanonical )
{
    FWorldPartition Partition;
    LUMEN_CHECK_HAS_VALUE( Partition.Initialize( MakeConfig() ) );
    const FWorldCellId CellA{ 7ULL };
    const FWorldCellId CellB{ 3ULL };
    LUMEN_CHECK_HAS_VALUE( Partition.TryAssignActor( ProbeActor( 0ULL ), CellA, ProbePersistent( 0ULL ) ) );
    LUMEN_CHECK_HAS_VALUE( Partition.TryAssignActor( ProbeActor( 1ULL ), CellA, ProbePersistent( 1ULL ) ) );
    LUMEN_CHECK_HAS_VALUE( Partition.TryAssignActor( ProbeActor( 2ULL ), CellB, ProbePersistent( 2ULL ) ) );

    LUMEN_CHECK_HAS_ERROR( Partition.TryAssignActor( ProbeActor( 0ULL ), CellB, ProbePersistent( 0ULL ) ) );
    LUMEN_CHECK_HAS_ERROR( Partition.TryAssignActor( ProbeActor( 3ULL ), CellB, ProbePersistent( 0ULL ) ) );
    LUMEN_CHECK_EQ( Partition.GetStats().Conflicts, 1ULL );

    LUMEN_CHECK_TRUE( Partition.IsActorDormant( ProbeActor( 0ULL ) ) );
    LUMEN_CHECK_EQ( Partition.FindActorCell( ProbeActor( 0ULL ) ).ValueOr( InvalidWorldCellId ), CellA );
    LUMEN_CHECK_EQ( Partition.FindActorPersistentId( ProbeActor( 2ULL ) ).ValueOr( InvalidPersistentActorId ),
                    ProbePersistent( 2ULL ) );
    LUMEN_CHECK_EQ( Partition.FindActorByPersistentId( ProbePersistent( 1ULL ) ).ValueOr( InvalidActorID ), ProbeActor( 1ULL ) );

    ActorID CellMembers[4]{};
    const USize Count = Partition.GetCellActors( CellA, TSpan<ActorID>( CellMembers, 4ULL ) );
    LUMEN_CHECK_EQ( Count, 2ULL );

    LUMEN_CHECK_HAS_ERROR( Partition.TryUnloadCell( CellA, 0ULL, 0ULL ) );
    LUMEN_CHECK_HAS_VALUE( Partition.TryLoadCell( CellA, 3ULL, 11ULL ) );
    LUMEN_CHECK_FALSE( Partition.IsActorDormant( ProbeActor( 0ULL ) ) );
    LUMEN_CHECK_HAS_ERROR( Partition.TryLoadCell( CellA, 4ULL, 12ULL ) );
    LUMEN_CHECK_HAS_VALUE( Partition.TryUnloadCell( CellA, 5ULL, 13ULL ) );
    LUMEN_CHECK_TRUE( Partition.IsActorDormant( ProbeActor( 1ULL ) ) );

    const TSpan<const FPartitionJournalRecord> Journal = Partition.GetJournal();
    LUMEN_CHECK_EQ( Journal.size(), 5ULL );
    LUMEN_CHECK_EQ( Journal[0ULL].Kind, EPartitionCommandKind::AssignActor );
    LUMEN_CHECK_EQ( Journal[0ULL].PersistentId, ProbePersistent( 0ULL ) );
    LUMEN_CHECK_EQ( Journal[3ULL].Kind, EPartitionCommandKind::LoadCell );
    LUMEN_CHECK_EQ( Journal[3ULL].FrameIndex, 3ULL );
    LUMEN_CHECK_EQ( Journal[3ULL].SenderSequence, 11ULL );
    LUMEN_CHECK_EQ( Journal[4ULL].Kind, EPartitionCommandKind::UnloadCell );

    LUMEN_CHECK_HAS_VALUE( Partition.TryUnassignActor( ProbeActor( 0ULL ) ) );
    LUMEN_CHECK_FALSE( Partition.IsActorDormant( ProbeActor( 0ULL ) ) );
    LUMEN_CHECK_HAS_ERROR( Partition.TryUnassignActor( ProbeActor( 0ULL ) ) );
    LUMEN_CHECK_EQ( Partition.GetStats().AssignedActorCount, 2ULL );

    Partition.Clear();
    LUMEN_CHECK_EQ( Partition.GetJournal().size(), 0ULL );
    LUMEN_CHECK_TRUE( Partition.IsInitialized() );
}

LUMEN_TEST( FWorldPartition, UnassignSwapRemoveKeepsMovedActorIndexValid )
{
    FWorldPartition Partition;
    LUMEN_CHECK_HAS_VALUE( Partition.Initialize( MakeConfig() ) );
    const FWorldCellId CellA{ 7ULL };
    const FWorldCellId CellB{ 3ULL };
    for ( USize Index = 0ULL; Index < 3ULL; ++Index )
    {
        LUMEN_CHECK_HAS_VALUE(
            Partition.TryAssignActor( ProbeActor( Index ), Index == 0ULL ? CellA : CellB, ProbePersistent( Index ) ) );
    }
    LUMEN_CHECK_HAS_VALUE( Partition.TryLoadCell( CellA, 1ULL, 0ULL ) );
    LUMEN_CHECK_HAS_VALUE( Partition.TryLoadCell( CellB, 1ULL, 0ULL ) );

    /** INFO: Removing slot 0 swaps the tail actor into it; every lookup must follow the moved entry. */
    LUMEN_CHECK_HAS_VALUE( Partition.TryUnassignActor( ProbeActor( 0ULL ) ) );
    LUMEN_CHECK_EQ( Partition.FindActorCell( ProbeActor( 2ULL ) ).ValueOr( InvalidWorldCellId ), CellB );
    LUMEN_CHECK_EQ( Partition.FindActorPersistentId( ProbeActor( 2ULL ) ).ValueOr( InvalidPersistentActorId ),
                    ProbePersistent( 2ULL ) );
    LUMEN_CHECK_EQ( Partition.FindActorByPersistentId( ProbePersistent( 2ULL ) ).ValueOr( InvalidActorID ), ProbeActor( 2ULL ) );
    LUMEN_CHECK_FALSE( Partition.IsActorDormant( ProbeActor( 2ULL ) ) );

    LUMEN_CHECK_HAS_VALUE( Partition.TryUnassignActor( ProbeActor( 2ULL ) ) );
    LUMEN_CHECK_FALSE( Partition.FindActorCell( ProbeActor( 2ULL ) ).HasValue() );
    LUMEN_CHECK_EQ( Partition.GetStats().AssignedActorCount, 1ULL );
}

LUMEN_TEST( FWorldPartition, CapacityBoundariesNeverMutateOnFailure )
{
    FWorldPartition Partition;
    LUMEN_CHECK_HAS_VALUE( Partition.Initialize( MakeConfig( 1ULL, 1ULL, 1ULL ) ) );
    LUMEN_CHECK_HAS_VALUE( Partition.TryAssignActor( ProbeActor( 0ULL ), FWorldCellId{ 1ULL }, ProbePersistent( 0ULL ) ) );
    LUMEN_CHECK_HAS_ERROR( Partition.TryAssignActor( ProbeActor( 1ULL ), FWorldCellId{ 1ULL }, ProbePersistent( 1ULL ) ) );
    LUMEN_CHECK_HAS_ERROR( Partition.TryAssignActor( ProbeActor( 1ULL ), FWorldCellId{ 2ULL }, ProbePersistent( 1ULL ) ) );
    LUMEN_CHECK_HAS_ERROR( Partition.TryAssignActor( ProbeActor( 0ULL ), FWorldCellId{ 1ULL }, ProbePersistent( 0ULL ) ) );
    LUMEN_CHECK_HAS_ERROR( Partition.TryLoadCell( FWorldCellId{ 1ULL }, 0ULL, 0ULL ) );
    LUMEN_CHECK_EQ( Partition.GetJournal().size(), 1ULL );
    LUMEN_CHECK_TRUE( Partition.IsActorDormant( ProbeActor( 0ULL ) ) );
    LUMEN_CHECK_EQ( Partition.GetStats().DroppedCommands, 0ULL );
}

LUMEN_TEST( FWorldPartitionAllocation, ReservedCommandsAndJournalDoNotAllocate )
{
    FWorldPartition Partition;
    LUMEN_CHECK_HAS_VALUE( Partition.Initialize( MakeConfig( 8ULL, 8ULL, 8192ULL ) ) );
    for ( USize Index = 0ULL; Index < 4ULL; ++Index )
    {
        LUMEN_CHECK_HAS_VALUE(
            Partition.TryAssignActor( ProbeActor( Index ), FWorldCellId{ Index + 1ULL }, ProbePersistent( Index ) ) );
        LUMEN_CHECK_HAS_VALUE( Partition.TryLoadCell( FWorldCellId{ Index + 1ULL }, 0ULL, 0ULL ) );
    }

    const FAllocationTrackingScope Tracking;
    for ( UInt32 Round = 0U; Round < 100U; ++Round )
    {
        for ( USize Index = 0ULL; Index < 4ULL; ++Index )
        {
            static_cast<void>( Partition.TryUnloadCell( FWorldCellId{ Index + 1ULL }, Round, 0ULL ) );
            static_cast<void>( Partition.TryLoadCell( FWorldCellId{ Index + 1ULL }, Round, 1ULL ) );
        }
    }
    LUMEN_CHECK_EQ( Tracking.GetCount(), 0ULL );
}

} // namespace Lumen
