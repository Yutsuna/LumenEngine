/**
 * @file WorldPartition.cpp
 * @brief Bounded logical cell registry implementation.
 */

#include <WorldPartition/WorldPartition.hpp>

#include <Templates/Unexpected.hpp>

Lumen::FWorldPartition::FWorldPartition () noexcept = default;

Lumen::FWorldPartition::~FWorldPartition () noexcept = default;

Lumen::TExpected<void, Lumen::EWorldPartitionError>
Lumen::FWorldPartition::Initialize ( const FWorldPartitionConfig &InConfig ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InConfig.Validate() );
    if ( bInitialized )
    {
        return MakeUnexpected( EWorldPartitionError::AlreadyInitialized );
    }
    if ( InConfig.MaxCells == 0ULL )
    {
        return {};
    }

    Cells.reserve( InConfig.MaxCells );
    Actors.reserve( InConfig.MaxAssignedActors );
    CellIndex.reserve( InConfig.MaxCells );
    ActorIndex.reserve( InConfig.MaxAssignedActors );
    PersistentToActor.reserve( InConfig.MaxAssignedActors );
    Journal.reserve( InConfig.MaxJournalRecords );

    MaxCells          = InConfig.MaxCells;
    MaxAssignedActors = InConfig.MaxAssignedActors;
    MaxJournalRecords = InConfig.MaxJournalRecords;
    bInitialized      = true;
    return {};
}

Lumen::Bool Lumen::FWorldPartition::IsInitialized () const noexcept
{
    return bInitialized;
}

Lumen::TExpected<void, Lumen::EWorldPartitionError>
Lumen::FWorldPartition::CanAssignActor ( const ActorID InActorId, const FWorldCellId InCellId ) const noexcept
{
    if ( not bInitialized )
    {
        return MakeUnexpected( EWorldPartitionError::NotInitialized );
    }
    if ( not InCellId.IsValid() )
    {
        return MakeUnexpected( EWorldPartitionError::InvalidCellId );
    }
    if ( ActorIndex.contains( InActorId ) )
    {
        return MakeUnexpected( EWorldPartitionError::ActorAlreadyAssigned );
    }
    if ( Actors.size() >= MaxAssignedActors )
    {
        return MakeUnexpected( EWorldPartitionError::CapacityExceeded );
    }
    if ( not CanAppendJournal() )
    {
        return MakeUnexpected( EWorldPartitionError::JournalFull );
    }
    const auto CellFound = CellIndex.find( InCellId.GetValue() );
    if ( CellFound == CellIndex.end() and Cells.size() >= MaxCells )
    {
        return MakeUnexpected( EWorldPartitionError::CapacityExceeded );
    }
    return {};
}

Lumen::TExpected<void, Lumen::EWorldPartitionError>
Lumen::FWorldPartition::CanUnassignActor ( const ActorID InActorId ) const noexcept
{
    if ( not bInitialized )
    {
        return MakeUnexpected( EWorldPartitionError::NotInitialized );
    }
    if ( not ActorIndex.contains( InActorId ) )
    {
        return MakeUnexpected( EWorldPartitionError::ActorNotAssigned );
    }
    if ( not CanAppendJournal() )
    {
        return MakeUnexpected( EWorldPartitionError::JournalFull );
    }
    return {};
}

Lumen::TExpected<void, Lumen::EWorldPartitionError> Lumen::FWorldPartition::TryAssignActor (
    const ActorID InActorId, const FWorldCellId InCellId, const FPersistentActorId InPersistentId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CanAssignActor( InActorId, InCellId ) );
    if ( not InPersistentId.IsValid() )
    {
        return MakeUnexpected( EWorldPartitionError::InvalidPersistentId );
    }
    const auto ExistingPersistent = PersistentToActor.find( InPersistentId.GetValue() );
    if ( ExistingPersistent != PersistentToActor.end() and ExistingPersistent->second != InActorId )
    {
        ++Stats.Conflicts;
        return MakeUnexpected( EWorldPartitionError::PersistentConflict );
    }

    USize CellSlot       = 0ULL;
    const auto CellFound = CellIndex.find( InCellId.GetValue() );
    if ( CellFound == CellIndex.end() )
    {
        CellSlot = Cells.size();
        Cells.push_back( FCellEntry{ .Id = InCellId } );
        CellIndex.emplace( InCellId.GetValue(), CellSlot );
    }
    else
    {
        CellSlot = CellFound->second;
    }

    const USize ActorSlot = Actors.size();
    Actors.push_back( FActorEntry{ .Id = InActorId, .CellId = InCellId, .PersistentId = InPersistentId } );
    ActorIndex.emplace( InActorId, ActorSlot );
    PersistentToActor.emplace( InPersistentId.GetValue(), InActorId );
    ++Cells[CellSlot].ActorCount;
    ++Stats.AssignedActorCount;

    return AppendJournal( EPartitionCommandKind::AssignActor, InCellId, InActorId, InPersistentId, 0ULL, 0ULL );
}

Lumen::TExpected<void, Lumen::EWorldPartitionError> Lumen::FWorldPartition::TryUnassignActor ( const ActorID InActorId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CanUnassignActor( InActorId ) );
    const auto ActorFound = ActorIndex.find( InActorId );
    /** INFO: Redundant guard: CanUnassignActor validated membership, but the optimiser cannot prove iterator validity. */
    if ( ActorFound == ActorIndex.end() )
    {
        return MakeUnexpected( EWorldPartitionError::ActorNotAssigned );
    }

    const USize Slot        = ActorFound->second;
    const FActorEntry Entry = Actors[Slot];
    const auto CellFound    = CellIndex.find( Entry.CellId.GetValue() );
    if ( CellFound != CellIndex.end() and Cells[CellFound->second].ActorCount > 0ULL )
    {
        --Cells[CellFound->second].ActorCount;
    }
    ActorIndex.erase( ActorFound );
    PersistentToActor.erase( Entry.PersistentId.GetValue() );

    const USize LastSlot = Actors.size() - 1ULL;
    if ( Slot != LastSlot )
    {
        Actors[Slot] = Actors[LastSlot];
        /** INFO: emplace would leave the moved actor pointing at the removed tail slot; overwrite the mapping instead. */
        ActorIndex[Actors[Slot].Id] = Slot;
    }
    Actors.pop_back();
    --Stats.AssignedActorCount;

    return AppendJournal( EPartitionCommandKind::UnassignActor, Entry.CellId, InActorId, Entry.PersistentId, 0ULL, 0ULL );
}

Lumen::TExpected<void, Lumen::EWorldPartitionError> Lumen::FWorldPartition::TryLoadCell ( const FWorldCellId InCellId,
                                                                                          const UInt64 InFrameIndex,
                                                                                          const UInt64 InSenderSequence ) noexcept
{
    if ( not bInitialized )
    {
        return MakeUnexpected( EWorldPartitionError::NotInitialized );
    }
    const auto CellFound = CellIndex.find( InCellId.GetValue() );
    if ( CellFound == CellIndex.end() )
    {
        return MakeUnexpected( EWorldPartitionError::UnknownCell );
    }
    FCellEntry &Cell = Cells[CellFound->second];
    if ( Cell.State == EWorldCellState::Loaded )
    {
        return MakeUnexpected( EWorldPartitionError::DuplicateLoad );
    }
    if ( not CanAppendJournal() )
    {
        return MakeUnexpected( EWorldPartitionError::JournalFull );
    }

    Cell.State = EWorldCellState::Loaded;
    ++Stats.LoadedCellCount;
    return AppendJournal( EPartitionCommandKind::LoadCell, InCellId, InvalidActorID, InvalidPersistentActorId, InFrameIndex,
                          InSenderSequence );
}

Lumen::TExpected<void, Lumen::EWorldPartitionError> Lumen::FWorldPartition::TryUnloadCell (
    const FWorldCellId InCellId, const UInt64 InFrameIndex, const UInt64 InSenderSequence ) noexcept
{
    if ( not bInitialized )
    {
        return MakeUnexpected( EWorldPartitionError::NotInitialized );
    }
    const auto CellFound = CellIndex.find( InCellId.GetValue() );
    if ( CellFound == CellIndex.end() )
    {
        return MakeUnexpected( EWorldPartitionError::UnknownCell );
    }
    FCellEntry &Cell = Cells[CellFound->second];
    if ( Cell.State != EWorldCellState::Loaded )
    {
        return MakeUnexpected( EWorldPartitionError::CellNotLoaded );
    }
    if ( not CanAppendJournal() )
    {
        return MakeUnexpected( EWorldPartitionError::JournalFull );
    }

    Cell.State = EWorldCellState::Unloaded;
    --Stats.LoadedCellCount;
    return AppendJournal( EPartitionCommandKind::UnloadCell, InCellId, InvalidActorID, InvalidPersistentActorId, InFrameIndex,
                          InSenderSequence );
}

Lumen::TOptional<Lumen::EWorldCellState> Lumen::FWorldPartition::FindCellState ( const FWorldCellId InCellId ) const noexcept
{
    const auto CellFound = CellIndex.find( InCellId.GetValue() );
    if ( CellFound == CellIndex.end() )
    {
        return {};
    }
    return TOptional<EWorldCellState>( Cells[CellFound->second].State );
}

Lumen::Bool Lumen::FWorldPartition::IsActorDormant ( const ActorID InActorId ) const noexcept
{
    if ( not bInitialized )
    {
        return false;
    }
    const auto ActorFound = ActorIndex.find( InActorId );
    if ( ActorFound == ActorIndex.end() )
    {
        return false;
    }
    const auto CellFound = CellIndex.find( Actors[ActorFound->second].CellId.GetValue() );
    return CellFound != CellIndex.end() and Cells[CellFound->second].State == EWorldCellState::Unloaded;
}

Lumen::TOptional<Lumen::FWorldCellId> Lumen::FWorldPartition::FindActorCell ( const ActorID InActorId ) const noexcept
{
    const auto ActorFound = ActorIndex.find( InActorId );
    if ( ActorFound == ActorIndex.end() )
    {
        return {};
    }
    return TOptional<FWorldCellId>( Actors[ActorFound->second].CellId );
}

Lumen::TOptional<Lumen::FPersistentActorId>
Lumen::FWorldPartition::FindActorPersistentId ( const ActorID InActorId ) const noexcept
{
    const auto ActorFound = ActorIndex.find( InActorId );
    if ( ActorFound == ActorIndex.end() )
    {
        return {};
    }
    return TOptional<FPersistentActorId>( Actors[ActorFound->second].PersistentId );
}

Lumen::TOptional<Lumen::ActorID>
Lumen::FWorldPartition::FindActorByPersistentId ( const FPersistentActorId InPersistentId ) const noexcept
{
    const auto Found = PersistentToActor.find( InPersistentId.GetValue() );
    if ( Found == PersistentToActor.end() )
    {
        return {};
    }
    return TOptional<ActorID>( Found->second );
}

Lumen::USize Lumen::FWorldPartition::GetCellActors ( const FWorldCellId InCellId, const TSpan<ActorID> OutActors ) const noexcept
{
    USize Written = 0ULL;
    for ( const FActorEntry &Entry : Actors )
    {
        if ( Entry.CellId != InCellId )
        {
            continue;
        }
        if ( Written >= OutActors.size() )
        {
            break;
        }
        OutActors[Written++] = Entry.Id;
    }
    return Written;
}

Lumen::TSpan<const Lumen::FPartitionJournalRecord> Lumen::FWorldPartition::GetJournal () const noexcept
{
    return { Journal.data(), Journal.size() };
}

Lumen::FWorldPartitionStats Lumen::FWorldPartition::GetStats () const noexcept
{
    FWorldPartitionStats Result = Stats;
    Result.JournalRecords       = Journal.size();
    return Result;
}

void Lumen::FWorldPartition::Clear () noexcept
{
    Cells.clear();
    Actors.clear();
    CellIndex.clear();
    ActorIndex.clear();
    PersistentToActor.clear();
    Journal.clear();
    Stats = {};
}

Lumen::Bool Lumen::FWorldPartition::CanAppendJournal () const noexcept
{
    return Journal.size() < MaxJournalRecords;
}

Lumen::TExpected<void, Lumen::EWorldPartitionError>
Lumen::FWorldPartition::AppendJournal ( const EPartitionCommandKind InKind,
                                        const FWorldCellId InCellId,
                                        const ActorID InActorId,
                                        const FPersistentActorId InPersistentId,
                                        const UInt64 InFrameIndex,
                                        const UInt64 InSenderSequence ) noexcept
{
    if ( not CanAppendJournal() )
    {
        ++Stats.DroppedCommands;
        return MakeUnexpected( EWorldPartitionError::JournalFull );
    }
    Journal.push_back( FPartitionJournalRecord{ .FrameIndex     = InFrameIndex,
                                                .Kind           = InKind,
                                                .CellId         = InCellId,
                                                .ActorId        = InActorId,
                                                .PersistentId   = InPersistentId,
                                                .SenderSequence = InSenderSequence } );
    return {};
}
