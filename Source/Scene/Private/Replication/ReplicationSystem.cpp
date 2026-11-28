/**
 * @file ReplicationSystem.cpp
 * @brief Implementation of the bounded authority/replica replication state machine.
 */

#include <Replication/ReplicationSystem.hpp>

#include <Algo/Sort.hpp>

#include <Templates/Unexpected.hpp>

#include <cstring>

Lumen::FReplicationSystem::FReplicationSystem () noexcept = default;

Lumen::FReplicationSystem::~FReplicationSystem () noexcept = default;

Lumen::TExpected<void, Lumen::EReplicationError>
Lumen::FReplicationSystem::Initialize ( const FReplicationConfig &InConfig,
                                        const EActorRole InRole,
                                        IReplicationTransport &InTransport,
                                        IReplicationActorHandler &InHandler ) noexcept
{
    if ( bInitialized )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::AlreadyInitialized };
    }
    TExpected<void, EReplicationError> Validated = InConfig.Validate();
    if ( not Validated )
    {
        return TUnexpected<EReplicationError>{ Validated.Error() };
    }

    Config    = InConfig;
    Role      = InRole;
    Transport = &InTransport;
    Handler   = &InHandler;
    LastError.Reset();

    OutBuilder.Reserve( Config.MaxPayloadBytes, Config.MaxRecordsPerFrame );
    InBuilder.Reserve( Config.MaxPayloadBytes, Config.MaxRecordsPerFrame );
    OutPacket.reserve( Config.MaxPayloadBytes );
    InPacket.reserve( Config.MaxPayloadBytes );
    OutPacket.resize( Config.MaxPayloadBytes );
    InPacket.resize( Config.MaxPayloadBytes );
    ResendBytes.reserve( Config.ResendWindow * Config.MaxPayloadBytes );
    ResendBytes.resize( Config.ResendWindow * Config.MaxPayloadBytes );
    ResendEntries.reserve( Config.ResendWindow );
    GapBytes.reserve( Config.MaxBufferedFrames * Config.MaxPayloadBytes );
    GapBytes.resize( Config.MaxBufferedFrames * Config.MaxPayloadBytes );
    GapEntries.reserve( Config.MaxBufferedFrames );
    PendingControl.reserve( Config.MaxPendingControlRecords );
    PendingParents.reserve( Config.MaxPendingParents );
    ReceivedInputs.reserve( Config.MaxPendingInputs );
    WorkingView.reserve( Config.MaxActors );
    SortedIds.reserve( Config.MaxActors );

    for ( USize Index = 0ULL; Index < 4ULL; ++Index )
    {
        Cursors[Index] = FReplicationChannelStats{};
    }
    Stats        = FReplicationStats{};
    bInitialized = true;
    return {};
}

Lumen::Bool Lumen::FReplicationSystem::IsInitialized () const noexcept
{
    return bInitialized;
}

Lumen::EActorRole Lumen::FReplicationSystem::GetRole () const noexcept
{
    return Role;
}

Lumen::TExpected<void, Lumen::EReplicationError>
Lumen::FReplicationSystem::AssignPersistentId ( const ActorID InLocalId ) noexcept
{
    if ( not bInitialized )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::NotInitialized };
    }
    if ( Role != EActorRole::Authority )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::AuthorityViolation };
    }
    if ( InLocalId == InvalidActorID )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::InvalidPersistentId };
    }
    if ( PersistentByLocal.find( InLocalId ) != PersistentByLocal.end() )
    {
        return {};
    }
    if ( LocalByPersistent.size() >= Config.MaxActors )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::CapacityExceeded };
    }
    const UInt64 Value = NextPersistentId++;
    LocalByPersistent.emplace( Value, InLocalId );
    PersistentByLocal.emplace( InLocalId, Value );
    Visibility.emplace( Value, true );
    return {};
}

Lumen::TExpected<void, Lumen::EReplicationError>
Lumen::FReplicationSystem::BindPersistentId ( const ActorID InLocalId, const FPersistentActorId InPersistentId ) noexcept
{
    if ( not bInitialized )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::NotInitialized };
    }
    if ( Role != EActorRole::Replica )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::AuthorityViolation };
    }
    if ( InLocalId == InvalidActorID or not InPersistentId.IsValid() )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::InvalidPersistentId };
    }
    const UInt64 Value = InPersistentId.GetValue();
    if ( LocalByPersistent.find( Value ) != LocalByPersistent.end() or
         PersistentByLocal.find( InLocalId ) != PersistentByLocal.end() )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::InvalidPersistentId };
    }
    if ( LocalByPersistent.size() >= Config.MaxActors )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::CapacityExceeded };
    }
    LocalByPersistent.emplace( Value, InLocalId );
    PersistentByLocal.emplace( InLocalId, Value );
    Visibility.emplace( Value, true );
    return {};
}

Lumen::TOptional<Lumen::ActorID> Lumen::FReplicationSystem::FindLocalId ( const FPersistentActorId InId ) const noexcept
{
    const auto Found = LocalByPersistent.find( InId.GetValue() );
    if ( Found == LocalByPersistent.end() )
    {
        return {};
    }
    return TOptional<ActorID>( Found->second );
}

Lumen::TOptional<Lumen::FPersistentActorId> Lumen::FReplicationSystem::FindPersistentId ( const ActorID InLocalId ) const noexcept
{
    const auto Found = PersistentByLocal.find( InLocalId );
    if ( Found == PersistentByLocal.end() )
    {
        return {};
    }
    return TOptional<FPersistentActorId>( FPersistentActorId( Found->second ) );
}

Lumen::TExpected<void, Lumen::EReplicationError> Lumen::FReplicationSystem::SetVisibility ( const FPersistentActorId InId,
                                                                                            const Bool bInVisible ) noexcept
{
    if ( not bInitialized )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::NotInitialized };
    }
    if ( Role != EActorRole::Authority )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::AuthorityViolation };
    }
    if ( not InId.IsValid() )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::InvalidPersistentId };
    }
    const auto Found = Visibility.find( InId.GetValue() );
    if ( Found == Visibility.end() )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::UnknownActor };
    }
    Found->second = bInVisible;
    return {};
}

Lumen::Bool Lumen::FReplicationSystem::GetVisibility ( const FPersistentActorId InId ) const noexcept
{
    const auto Found = Visibility.find( InId.GetValue() );
    return Found == Visibility.end() ? true : Found->second;
}

Lumen::FReplicationStats Lumen::FReplicationSystem::GetStats () const noexcept
{
    FReplicationStats Snapshot = Stats;
    for ( USize DirectionIndex = 0ULL; DirectionIndex < 2ULL; ++DirectionIndex )
    {
        for ( USize ChannelIndex = 0ULL; ChannelIndex < 2ULL; ++ChannelIndex )
        {
            Snapshot.Channels[DirectionIndex][ChannelIndex] = Cursors[ReplicationChannelIndex(
                static_cast<EReplicationDirection>( DirectionIndex ), static_cast<EReplicationChannel>( ChannelIndex ) )];
        }
    }
    return Snapshot;
}

Lumen::TOptional<Lumen::EReplicationError> Lumen::FReplicationSystem::GetLastError () const noexcept
{
    return LastError;
}

Lumen::USize Lumen::FReplicationSystem::CopyReceivedInputs ( TSpan<FReplicationInputRecord> OutRecords ) noexcept
{
    const USize Count = ReceivedInputs.size() < OutRecords.size() ? ReceivedInputs.size() : OutRecords.size();
    for ( USize Index = 0ULL; Index < Count; ++Index )
    {
        OutRecords[Index] = ReceivedInputs[Index];
    }
    ReceivedInputs.erase( ReceivedInputs.begin(), ReceivedInputs.begin() + static_cast<ISize>( Count ) );
    return Count;
}

void Lumen::FReplicationSystem::BuildWorkingView ( const FReplicationSourceView &InSource ) noexcept
{
    WorkingView.clear();
    for ( const FReplicationActorSourceRecord &Record : InSource.Actors )
    {
        if ( PersistentByLocal.find( Record.LocalId ) == PersistentByLocal.end() and Role == EActorRole::Authority )
        {
            static_cast<void>( AssignPersistentId( Record.LocalId ) );
        }
        const auto Found = PersistentByLocal.find( Record.LocalId );
        if ( Found == PersistentByLocal.end() )
        {
            continue;
        }
        UInt64 ParentValue = 0ULL;
        if ( Record.ParentLocalId != InvalidActorID )
        {
            const auto ParentFound = PersistentByLocal.find( Record.ParentLocalId );
            if ( ParentFound != PersistentByLocal.end() )
            {
                ParentValue = ParentFound->second;
            }
        }
        FWorkingActor Actor;
        Actor.PersistentId   = FPersistentActorId( Found->second );
        Actor.LocalId        = Record.LocalId;
        Actor.ParentId       = FPersistentActorId( ParentValue );
        Actor.LocalTransform = Record.LocalTransform;
        Actor.bVisible       = GetVisibility( Actor.PersistentId );
        WorkingView.push_back( Actor );
    }
}

void Lumen::FReplicationSystem::QueueControl ( const EReplicationControlKind InKind,
                                               const EReplicationDirection InDirection,
                                               const EReplicationChannel InChannel,
                                               const UInt64 InSequence ) noexcept
{
    if ( PendingControl.size() >= PendingControl.capacity() )
    {
        PendingControl.erase( PendingControl.begin() );
        ++Stats.OverflowFrameDropCount;
    }
    FReplicationRecord Record;
    Record.Kind         = EActorReplicationKind::Control;
    Record.PropertyRank = 0U;
    Record.SetControl(
        FReplicationControlPayload{ .Kind = InKind, .Direction = InDirection, .Channel = InChannel, .Sequence = InSequence } );
    PendingControl.push_back( Record );
    ++ControlRecordsQueuedThisTick;
}

void Lumen::FReplicationSystem::UpdateCursorsFromAck ( const EReplicationDirection InDirection,
                                                       const EReplicationChannel InChannel,
                                                       const UInt64 InAckedSequence ) noexcept
{
    FReplicationChannelStats &Cursor = Cursors[ReplicationChannelIndex( InDirection, InChannel )];
    if ( InAckedSequence <= Cursor.AckedSequence )
    {
        return;
    }
    Cursor.AckedSequence = InAckedSequence;
    if ( InDirection == EReplicationDirection::StateToReplica and InChannel == EReplicationChannel::Reliable )
    {
        Stats.LastAcknowledgedReliableSequence = InAckedSequence;
    }
}

void Lumen::FReplicationSystem::ApplyFrameToShadow ( const FReplicationFrame &InFrame,
                                                     TMap<UInt64, FActorState> &InOutShadow ) noexcept
{
    for ( const FReplicationRecord &Record : InFrame.Records )
    {
        const UInt64 Persistent = Record.Actor.GetValue();
        if ( Record.Kind == EActorReplicationKind::Spawn )
        {
            FActorState &State = InOutShadow[Persistent];
            State              = FActorState{};
            State.bAlive       = true;
        }
        else if ( Record.Kind == EActorReplicationKind::Destroy )
        {
            InOutShadow.erase( Persistent );
        }
        else if ( Record.Kind == EActorReplicationKind::Transform )
        {
            InOutShadow[Persistent].LocalTransform = Record.GetLocalTransform();
        }
        else if ( Record.Kind == EActorReplicationKind::Parent )
        {
            InOutShadow[Persistent].Parent = Record.GetParent();
        }
        else if ( Record.Kind == EActorReplicationKind::Visibility )
        {
            InOutShadow[Persistent].bVisible = Record.GetVisibility();
        }
    }
}

Lumen::TExpected<void, Lumen::EReplicationError> Lumen::FReplicationSystem::SendControlFrame () noexcept
{
    OutBuilder.Reset();
    for ( const FReplicationRecord &Record : PendingControl )
    {
        static_cast<void>( OutBuilder.TryAppend( Record ) );
    }
    if ( OutBuilder.GetRecords().empty() )
    {
        return {};
    }
    FReplicationFrameHeader Header;
    Header.Direction   = EReplicationDirection::StateToReplica;
    Header.Channel     = EReplicationChannel::Reliable;
    Header.FrameId     = CurrentTick;
    Header.Flags       = ReplicationFrameControlOnly;
    Header.RecordCount = static_cast<UInt16>( OutBuilder.GetRecords().size() );
    Header.ByteLength =
        static_cast<UInt32>( ReplicationFrameHeaderBytes + ( OutBuilder.GetRecords().size() * ReplicationFrameRecordBytes ) +
                             ReplicationFrameChecksumBytes );
    TExpected<USize, EReplicationError> Written = OutBuilder.TryWrite( OutPacket, Header );
    if ( not Written )
    {
        return TUnexpected<EReplicationError>{ Written.Error() };
    }
    TExpected<void, EReplicationError> Sent = Transport->TrySend( { OutPacket.data(), *Written } );
    if ( not Sent )
    {
        return TUnexpected<EReplicationError>{ Sent.Error() };
    }
    ++Stats.FramesBuilt;
    Cursors[ReplicationChannelIndex( EReplicationDirection::StateToReplica, EReplicationChannel::Reliable )].SentCount++;
    PendingControl.clear();
    return {};
}

Lumen::TExpected<void, Lumen::EReplicationError>
Lumen::FReplicationSystem::ProcessControlRecord ( const FReplicationRecord &InRecord ) noexcept
{
    const FReplicationControlPayload Control = InRecord.GetControl();
    switch ( Control.Kind )
    {
    case EReplicationControlKind::Ack:
        UpdateCursorsFromAck( Control.Direction, Control.Channel, Control.Sequence );
        if ( Control.Direction == EReplicationDirection::StateToReplica and Control.Channel == EReplicationChannel::Reliable )
        {
            for ( USize Index = 0ULL; Index < ResendEntries.size(); )
            {
                if ( ResendEntries[Index].Sequence <= Control.Sequence )
                {
                    const FResendEntry Entry = ResendEntries[Index];
                    if ( Entry.Length <= ResendBytes.size() )
                    {
                        TExpected<FReplicationFrame, EReplicationError> Parsed =
                            InBuilder.TryParse( { ResendBytes.data() + ( Index * Config.MaxPayloadBytes ), Entry.Length } );
                        if ( Parsed )
                        {
                            ApplyFrameToShadow( *Parsed, AckedShadow );
                        }
                    }
                    ResendEntries.erase( ResendEntries.begin() + static_cast<ISize>( Index ) );
                }
                else
                {
                    ++Index;
                }
            }
        }
        break;
    case EReplicationControlKind::Nack:
        for ( FResendEntry &Entry : ResendEntries )
        {
            if ( Entry.Sequence >= Control.Sequence )
            {
                Entry.Attempts = 0U;
            }
        }
        break;
    case EReplicationControlKind::BaselineRequest:
        if ( Role == EActorRole::Authority )
        {
            bForceFullSnapshot = true;
            QueueControl( EReplicationControlKind::BaselineReady, EReplicationDirection::StateToReplica,
                          EReplicationChannel::Reliable, Control.Sequence );
        }
        break;
    case EReplicationControlKind::BaselineReady:
        break;
    default:
        return TUnexpected<EReplicationError>{ EReplicationError::MalformedFrame };
    }
    return {};
}

Lumen::TOptional<Lumen::FReplicationFrame>
Lumen::FReplicationSystem::TryRecoverBufferedFrame ( const UInt64 InExpectedSequence ) noexcept
{
    for ( USize Index = 0ULL; Index < GapEntries.size(); ++Index )
    {
        if ( GapEntries[Index].Sequence != InExpectedSequence )
        {
            continue;
        }
        const FGapEntry Entry = GapEntries[Index];
        TExpected<FReplicationFrame, EReplicationError> Parsed =
            InBuilder.TryParse( { GapBytes.data() + ( Index * Config.MaxPayloadBytes ), Entry.Length } );
        if ( Parsed )
        {
            GapEntries.erase( GapEntries.begin() + static_cast<ISize>( Index ) );
            return TOptional<FReplicationFrame>( *Parsed );
        }
        return {};
    }
    return {};
}

void Lumen::FReplicationSystem::AppendAbsoluteFields ( const FWorkingActor &InActor,
                                                       FReplicationFrameBuilder &InOutBuilder ) noexcept
{
    FReplicationRecord Transform;
    Transform.Actor        = InActor.PersistentId;
    Transform.Kind         = EActorReplicationKind::Transform;
    Transform.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::LocalTransform );
    Transform.SetLocalTransform( InActor.LocalTransform );
    static_cast<void>( InOutBuilder.TryAppend( Transform ) );

    FReplicationRecord Parent;
    Parent.Actor        = InActor.PersistentId;
    Parent.Kind         = EActorReplicationKind::Parent;
    Parent.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::Parent );
    Parent.SetParent( InActor.ParentId );
    static_cast<void>( InOutBuilder.TryAppend( Parent ) );

    FReplicationRecord VisibilityRecord;
    VisibilityRecord.Actor        = InActor.PersistentId;
    VisibilityRecord.Kind         = EActorReplicationKind::Visibility;
    VisibilityRecord.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::Visibility );
    VisibilityRecord.SetVisibility( InActor.bVisible );
    static_cast<void>( InOutBuilder.TryAppend( VisibilityRecord ) );
}

Lumen::TExpected<void, Lumen::EReplicationError>
Lumen::FReplicationSystem::ApplyStateFrame ( const FReplicationFrame &InFrame, FReplicationTickResult &OutResult ) noexcept
{
    for ( const FReplicationRecord &Record : InFrame.Records )
    {
        const FPersistentActorId Persistent = Record.Actor;
        const UInt64 Value                  = Persistent.GetValue();
        switch ( Record.Kind )
        {
        case EActorReplicationKind::Spawn:
        {
            if ( FindLocalId( Persistent ) )
            {
                break;
            }
            TExpected<ActorID, EReplicationError> Spawned = Handler->TrySpawn( Persistent );
            if ( not Spawned )
            {
                return TUnexpected<EReplicationError>{ Spawned.Error() };
            }
            static_cast<void>( BindPersistentId( *Spawned, Persistent ) );
            FActorState &State = AppliedShadow[Value];
            State              = FActorState{};
            State.bAlive       = true;
            ++OutResult.RecordsApplied;
            break;
        }
        case EActorReplicationKind::Destroy:
        {
            const TOptional<ActorID> Local = FindLocalId( Persistent );
            if ( Local )
            {
                TExpected<void, EReplicationError> Destroyed = Handler->TryDestroy( *Local );
                if ( not Destroyed )
                {
                    return TUnexpected<EReplicationError>{ Destroyed.Error() };
                }
                PersistentByLocal.erase( *Local );
            }
            LocalByPersistent.erase( Value );
            AppliedShadow.erase( Value );
            Visibility.erase( Value );
            ++OutResult.RecordsApplied;
            break;
        }
        case EActorReplicationKind::Transform:
        {
            const TOptional<ActorID> Local = FindLocalId( Persistent );
            if ( Local )
            {
                TExpected<void, EReplicationError> Applied = Handler->TrySetLocalTransform( *Local, Record.GetLocalTransform() );
                if ( not Applied )
                {
                    return TUnexpected<EReplicationError>{ Applied.Error() };
                }
            }
            AppliedShadow[Value].LocalTransform = Record.GetLocalTransform();
            ++OutResult.RecordsApplied;
            break;
        }
        case EActorReplicationKind::Parent:
        {
            const TOptional<ActorID> Local = FindLocalId( Persistent );
            if ( not Local )
            {
                break;
            }
            const FPersistentActorId Parent      = Record.GetParent();
            const TOptional<ActorID> ParentLocal = Parent.IsValid() ? FindLocalId( Parent ) : TOptional<ActorID>{};
            if ( Parent.IsValid() and not ParentLocal )
            {
                if ( PendingParents.size() >= PendingParents.capacity() )
                {
                    PendingParents.erase( PendingParents.begin() );
                    ++Stats.PendingParentDrops;
                }
                PendingParents.push_back( FPendingParent{ .Child = Persistent, .Parent = Parent, .Ticks = 0U } );
                break;
            }
            static_cast<void>( Handler->TrySetParent( *Local, ParentLocal ? *ParentLocal : InvalidActorID ) );
            AppliedShadow[Value].Parent = Parent;
            ++OutResult.RecordsApplied;
            break;
        }
        case EActorReplicationKind::Visibility:
        {
            const auto Found = Visibility.find( Value );
            if ( Found != Visibility.end() )
            {
                Found->second = Record.GetVisibility();
            }
            AppliedShadow[Value].bVisible = Record.GetVisibility();
            ++OutResult.RecordsApplied;
            break;
        }
        case EActorReplicationKind::Input:
        {
            if ( Role != EActorRole::Authority )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::AuthorityViolation };
            }
            FReplicationInputRecord Input;
            Input.Actor                     = Persistent;
            Input.InputSequence             = Record.GetInputSequence();
            const TSpan<const Byte> Payload = Record.GetInputPayload();
            Input.PayloadSize               = static_cast<UInt16>( Payload.size() );
            for ( USize Index = 0ULL; Index < Payload.size(); ++Index )
            {
                Input.Payload[Index] = Payload[Index];
            }
            if ( ReceivedInputs.size() < ReceivedInputs.capacity() )
            {
                ReceivedInputs.push_back( Input );
            }
            else
            {
                ++Stats.OverflowFrameDropCount;
            }
            break;
        }
        case EActorReplicationKind::Control:
        default:
            break;
        }
    }
    return {};
}

Lumen::TExpected<void, Lumen::EReplicationError>
Lumen::FReplicationSystem::ProcessFrame ( const FReplicationFrame &InFrame, FReplicationTickResult &OutResult ) noexcept
{
    const EReplicationDirection Direction = InFrame.Header.Direction;
    const EReplicationChannel Channel     = InFrame.Header.Channel;
    FReplicationChannelStats &Cursor      = Cursors[ReplicationChannelIndex( Direction, Channel )];

    if ( ( InFrame.Header.Flags & ReplicationFrameControlOnly ) != 0U )
    {
        for ( const FReplicationRecord &Record : InFrame.Records )
        {
            if ( Record.Kind != EActorReplicationKind::Control )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::MalformedFrame };
            }
            TExpected<void, EReplicationError> Result = ProcessControlRecord( Record );
            if ( not Result )
            {
                return Result;
            }
        }
        return {};
    }

    if ( Channel == EReplicationChannel::Unreliable )
    {
        if ( InFrame.Header.Sequence <= Cursor.LastSeenSequence )
        {
            ++Stats.DuplicateFramesSuppressed;
            ++Cursor.DroppedCount;
            return {};
        }
        Cursor.LastSeenSequence = InFrame.Header.Sequence;
        ++Cursor.ReceivedCount;
        return ApplyStateFrame( InFrame, OutResult );
    }

    if ( Direction == EReplicationDirection::StateToReplica and Role != EActorRole::Replica )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::AuthorityViolation };
    }
    if ( Direction == EReplicationDirection::InputToAuthority and Role != EActorRole::Authority )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::AuthorityViolation };
    }

    const UInt64 Expected = Cursor.LastSeenSequence + 1ULL;
    if ( InFrame.Header.Sequence <= Cursor.LastSeenSequence )
    {
        ++Stats.DuplicateFramesSuppressed;
        ++Cursor.DroppedCount;
        return {};
    }
    if ( InFrame.Header.Sequence > Expected )
    {
        ++Stats.OutOfOrderRejected;
        QueueControl( EReplicationControlKind::Nack, Direction, Channel, Expected );
        if ( GapEntries.size() < GapEntries.capacity() and InFrame.Header.ByteLength <= Config.MaxPayloadBytes )
        {
            GapEntries.push_back( FGapEntry{ .Sequence = InFrame.Header.Sequence, .Length = InFrame.Header.ByteLength } );
            std::memcpy( GapBytes.data() + ( ( GapEntries.size() - 1ULL ) * Config.MaxPayloadBytes ), InPacket.data(),
                         InFrame.Header.ByteLength );
        }
        return {};
    }

    TExpected<void, EReplicationError> Applied = ApplyStateFrame( InFrame, OutResult );
    if ( not Applied )
    {
        return Applied;
    }
    Cursor.LastSeenSequence = InFrame.Header.Sequence;
    ++Cursor.ReceivedCount;
    ++Stats.FramesApplied;
    if ( Direction == EReplicationDirection::StateToReplica )
    {
        Stats.LastAppliedReliableSequence = InFrame.Header.Sequence;
        QueueControl( EReplicationControlKind::Ack, Direction, Channel, InFrame.Header.Sequence );
    }

    UInt64 Next = Cursor.LastSeenSequence + 1ULL;
    while ( true )
    {
        const TOptional<FReplicationFrame> Recovered = TryRecoverBufferedFrame( Next );
        if ( not Recovered )
        {
            break;
        }
        TExpected<void, EReplicationError> Buffered = ApplyStateFrame( *Recovered, OutResult );
        if ( not Buffered )
        {
            return Buffered;
        }
        Cursor.LastSeenSequence = Next;
        ++Stats.FramesApplied;
        if ( Direction == EReplicationDirection::StateToReplica )
        {
            Stats.LastAppliedReliableSequence = Next;
            QueueControl( EReplicationControlKind::Ack, Direction, Channel, Next );
        }
        ++Next;
    }
    return {};
}

void Lumen::FReplicationSystem::TryResolvePendingParents () noexcept
{
    for ( USize Index = 0ULL; Index < PendingParents.size(); )
    {
        FPendingParent &Pending         = PendingParents[Index];
        const TOptional<ActorID> Child  = FindLocalId( Pending.Child );
        const TOptional<ActorID> Parent = FindLocalId( Pending.Parent );
        if ( not Child )
        {
            PendingParents.erase( PendingParents.begin() + static_cast<ISize>( Index ) );
            continue;
        }
        if ( Parent )
        {
            static_cast<void>( Handler->TrySetParent( *Child, *Parent ) );
            PendingParents.erase( PendingParents.begin() + static_cast<ISize>( Index ) );
            continue;
        }
        if ( ++Pending.Ticks > Config.MaxPendingParentTicks )
        {
            PendingParents.erase( PendingParents.begin() + static_cast<ISize>( Index ) );
            ++Stats.PendingParentDrops;
            continue;
        }
        ++Index;
    }
}

Lumen::TExpected<Lumen::FReplicationTickResult, Lumen::EReplicationError>
Lumen::FReplicationSystem::TickAuthority ( const UInt64 InFrameIndex, FReplicationTickResult &OutResult ) noexcept
{
    FReplicationChannelStats &Cursor =
        Cursors[ReplicationChannelIndex( EReplicationDirection::StateToReplica, EReplicationChannel::Reliable )];
    const UInt64 Acked = Cursor.AckedSequence;

    for ( USize Index = 0ULL; Index < ResendEntries.size(); )
    {
        FResendEntry &Entry = ResendEntries[Index];
        if ( Entry.LastTick == CurrentTick )
        {
            ++Index;
            continue;
        }
        if ( Entry.Attempts >= Config.MaxResendAttempts or ( CurrentTick - Entry.LastTick ) >= Config.FullSnapshotTickThreshold )
        {
            ++Stats.StaleBaselineEscalations;
            QueueControl( EReplicationControlKind::BaselineRequest, EReplicationDirection::StateToReplica,
                          EReplicationChannel::Reliable, Entry.Sequence );
            ResendEntries.erase( ResendEntries.begin() + static_cast<ISize>( Index ) );
            continue;
        }
        if ( Entry.Length <= ResendBytes.size() )
        {
            static_cast<void>( Transport->TrySend( { ResendBytes.data() + ( Index * Config.MaxPayloadBytes ), Entry.Length } ) );
        }
        ++Entry.Attempts;
        Entry.LastTick = CurrentTick;
        ++Index;
    }

    OutBuilder.Reset();
    const Bool bFullSnapshot = bForceFullSnapshot or Acked == 0ULL;
    SortedIds.clear();
    for ( const FWorkingActor &Actor : WorkingView )
    {
        SortedIds.push_back( Actor.PersistentId.GetValue() );
    }
    Algo::Sort( SortedIds );

    /** INFO: A bound actor missing from the view is destroyed until its tombstone is acked. */
    const auto IsPresent = [this] ( const UInt64 InValue ) noexcept -> Bool
    {
        USize Lower = 0ULL;
        USize Upper = SortedIds.size();
        while ( Lower < Upper )
        {
            const USize Middle = Lower + ( ( Upper - Lower ) / 2ULL );
            if ( SortedIds[Middle] < InValue )
            {
                Lower = Middle + 1ULL;
            }
            else
            {
                Upper = Middle;
            }
        }
        return Lower < SortedIds.size() and SortedIds[Lower] == InValue;
    };
    for ( const auto &Entry : LocalByPersistent )
    {
        const UInt64 Value = Entry.first;
        if ( IsPresent( Value ) or AckedShadow.find( Value ) == AckedShadow.end() )
        {
            continue;
        }
        FReplicationRecord Destroy;
        Destroy.Actor        = FPersistentActorId( Value );
        Destroy.Kind         = EActorReplicationKind::Destroy;
        Destroy.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::Alive );
        static_cast<void>( OutBuilder.TryAppend( Destroy ) );
    }

    for ( const UInt64 Value : SortedIds )
    {
        FWorkingActor *FoundActor = nullptr;
        for ( FWorkingActor &Actor : WorkingView )
        {
            if ( Actor.PersistentId.GetValue() == Value )
            {
                FoundActor = &Actor;
                break;
            }
        }
        if ( FoundActor == nullptr )
        {
            continue;
        }
        const auto Shadow = AckedShadow.find( Value );
        const Bool bKnown = Shadow != AckedShadow.end() and Shadow->second.bAlive;
        if ( not bKnown )
        {
            FReplicationRecord Spawn;
            Spawn.Actor        = FoundActor->PersistentId;
            Spawn.Kind         = EActorReplicationKind::Spawn;
            Spawn.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::Alive );
            static_cast<void>( OutBuilder.TryAppend( Spawn ) );
        }
        if ( bFullSnapshot or not bKnown )
        {
            AppendAbsoluteFields( *FoundActor, OutBuilder );
            continue;
        }
        if ( FoundActor->LocalTransform != Shadow->second.LocalTransform )
        {
            FReplicationRecord Record;
            Record.Actor        = FoundActor->PersistentId;
            Record.Kind         = EActorReplicationKind::Transform;
            Record.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::LocalTransform );
            Record.SetLocalTransform( FoundActor->LocalTransform );
            static_cast<void>( OutBuilder.TryAppend( Record ) );
        }
        if ( FoundActor->ParentId != Shadow->second.Parent )
        {
            FReplicationRecord Record;
            Record.Actor        = FoundActor->PersistentId;
            Record.Kind         = EActorReplicationKind::Parent;
            Record.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::Parent );
            Record.SetParent( FoundActor->ParentId );
            static_cast<void>( OutBuilder.TryAppend( Record ) );
        }
        if ( FoundActor->bVisible != Shadow->second.bVisible )
        {
            FReplicationRecord Record;
            Record.Actor        = FoundActor->PersistentId;
            Record.Kind         = EActorReplicationKind::Visibility;
            Record.PropertyRank = static_cast<UInt8>( EActorReplicationProperty::Visibility );
            Record.SetVisibility( FoundActor->bVisible );
            static_cast<void>( OutBuilder.TryAppend( Record ) );
        }
    }

    if ( OutBuilder.GetRecords().empty() )
    {
        bForceFullSnapshot = false;
        return FReplicationTickResult( OutResult );
    }
    if ( OutBuilder.GetRecords().size() > Config.MaxRecordsPerFrame )
    {
        ++Stats.OverflowFrameDropCount;
        OutResult.bOverflow = true;
        if ( not ResendEntries.empty() )
        {
            ResendEntries.erase( ResendEntries.begin() );
        }
        return FReplicationTickResult( OutResult );
    }

    FReplicationFrameHeader Header;
    Header.Sequence         = NextStateSequence++;
    Header.AckedSequence    = Acked;
    Header.BaselineSequence = Acked;
    Header.FrameId          = InFrameIndex;
    Header.Direction        = EReplicationDirection::StateToReplica;
    Header.Channel          = EReplicationChannel::Reliable;
    Header.RecordCount      = static_cast<UInt16>( OutBuilder.GetRecords().size() );
    Header.Flags            = bFullSnapshot ? ReplicationFrameFullSnapshot : 0U;
    Header.ByteLength =
        static_cast<UInt32>( ReplicationFrameHeaderBytes + ( OutBuilder.GetRecords().size() * ReplicationFrameRecordBytes ) +
                             ReplicationFrameChecksumBytes );
    TExpected<USize, EReplicationError> Written = OutBuilder.TryWrite( OutPacket, Header );
    if ( not Written )
    {
        ++Stats.OverflowFrameDropCount;
        OutResult.bOverflow = true;
        return TUnexpected<EReplicationError>{ Written.Error() };
    }
    TExpected<void, EReplicationError> Sent = Transport->TrySend( { OutPacket.data(), *Written } );
    if ( not Sent )
    {
        LastError.Emplace( Sent.Error() );
        return TUnexpected<EReplicationError>{ Sent.Error() };
    }
    ++Stats.FramesBuilt;
    ++OutResult.FramesSent;
    ++Cursor.SentCount;
    if ( bFullSnapshot )
    {
        ++Stats.FullSnapshotsSent;
        LastFullSnapshotTick = CurrentTick;
        bForceFullSnapshot   = false;
    }

    if ( ResendEntries.size() >= ResendEntries.capacity() )
    {
        ResendEntries.erase( ResendEntries.begin() );
        ++Stats.DroppedResendCount;
    }
    const USize Slot = ResendEntries.size() * Config.MaxPayloadBytes;
    if ( Slot + *Written <= ResendBytes.size() )
    {
        std::memcpy( ResendBytes.data() + Slot, OutPacket.data(), *Written );
        ResendEntries.push_back( FResendEntry{
            .Sequence = Header.Sequence, .FrameId = InFrameIndex, .LastTick = CurrentTick, .Length = *Written, .Attempts = 0U } );
    }
    return FReplicationTickResult( OutResult );
}

Lumen::TExpected<Lumen::FReplicationTickResult, Lumen::EReplicationError> Lumen::FReplicationSystem::TickReplica (
    const UInt64, const FReplicationSourceView &, FReplicationTickResult &OutResult ) noexcept
{
    return FReplicationTickResult( OutResult );
}

Lumen::TExpected<Lumen::FReplicationTickResult, Lumen::EReplicationError>
Lumen::FReplicationSystem::Tick ( const UInt64 InFrameIndex, const FReplicationSourceView &InSource ) noexcept
{
    if ( not bInitialized )
    {
        return TUnexpected<EReplicationError>{ EReplicationError::NotInitialized };
    }
    ++CurrentTick;
    FReplicationTickResult Result;
    ControlRecordsQueuedThisTick = 0ULL;

    InPacket.resize( Config.MaxPayloadBytes );
    while ( true )
    {
        TExpected<USize, EReplicationError> Received = Transport->TryReceive( { InPacket.data(), InPacket.size() } );
        if ( not Received )
        {
            LastError.Emplace( Received.Error() );
            break;
        }
        if ( *Received == 0ULL )
        {
            break;
        }
        TExpected<FReplicationFrame, EReplicationError> Parsed = InBuilder.TryParse( { InPacket.data(), *Received } );
        if ( not Parsed )
        {
            ++Stats.FramesDropped;
            LastError.Emplace( Parsed.Error() );
            continue;
        }
        ++Result.FramesReceived;
        TExpected<void, EReplicationError> Processed = ProcessFrame( *Parsed, Result );
        if ( not Processed )
        {
            LastError.Emplace( Processed.Error() );
        }
    }

    BuildWorkingView( InSource );
    if ( Role == EActorRole::Authority )
    {
        TExpected<FReplicationTickResult, EReplicationError> AuthorityResult = TickAuthority( InFrameIndex, Result );
        if ( not AuthorityResult )
        {
            LastError.Emplace( AuthorityResult.Error() );
        }
    }
    else if ( Role == EActorRole::Replica )
    {
        static_cast<void>( TickReplica( InFrameIndex, InSource, Result ) );
    }

    TryResolvePendingParents();
    if ( not PendingControl.empty() )
    {
        TExpected<void, EReplicationError> ControlResult = SendControlFrame();
        if ( not ControlResult )
        {
            LastError.Emplace( ControlResult.Error() );
        }
    }
    Result.ControlRecordsSent = ControlRecordsQueuedThisTick;
    return FReplicationTickResult( Result );
}
