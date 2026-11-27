/**
 * @file SceneHierarchy.cpp
 * @brief Implementation of FSceneHierarchy.
 */

#include <Hierarchy/SceneHierarchy.hpp>

#include <Core/CoreDefinitions.hpp>

#include <limits>
#include <new>

/**
 * Public
 */

struct Lumen::FSceneHierarchy::FNode
{
    ActorID Id                     = InvalidActorID;
    ActorID ParentId               = InvalidActorID;
    ActorID FirstChildId           = InvalidActorID;
    ActorID LastChildId            = InvalidActorID;
    ActorID PreviousSiblingId      = InvalidActorID;
    ActorID NextSiblingId          = InvalidActorID;
    Maths::FMat4x4f LocalTransform = Maths::FMat4x4f::Identity();
    Maths::FMat4x4f WorldTransform = Maths::FMat4x4f::Identity();
    USize Depth                    = 0ULL;
    Bool bWorldDirty               = false;
    Bool bWorldUpdated             = false;
    Bool bPublicationDirty         = false;
};

Lumen::FSceneHierarchy::FSceneHierarchy () noexcept = default;

Lumen::FSceneHierarchy::~FSceneHierarchy () noexcept = default;

Lumen::TExpected<void, Lumen::ESceneHierarchyError> Lumen::FSceneHierarchy::Initialize ( const USize InCapacity ) noexcept
{
    if ( IsInitialized() )
    {
        return MakeUnexpected( ESceneHierarchyError::AlreadyInitialized );
    }

    if ( InCapacity == 0ULL or InCapacity > std::numeric_limits<USize>::max() / sizeof( FNode ) or
         InCapacity > std::numeric_limits<USize>::max() / sizeof( FSceneHierarchyTransformRecord ) )
    {
        return MakeUnexpected( ESceneHierarchyError::InvalidCapacity );
    }

    TUniquePtr<FNode[]> NewNodes( new ( std::nothrow ) FNode[InCapacity]() );
    TUniquePtr<FSceneHierarchyTransformRecord[]> NewPublishedTransforms( new ( std::nothrow )
                                                                             FSceneHierarchyTransformRecord[InCapacity]() );
    TUniquePtr<FSceneHierarchyTransformRecord[]> NewLevelOrdered( new ( std::nothrow )
                                                                      FSceneHierarchyTransformRecord[InCapacity]() );
    TUniquePtr<USize[]> NewLevelStarts( new ( std::nothrow ) USize[InCapacity + 1ULL]() );
    TUniquePtr<USize[]> NewLevelCursor( new ( std::nothrow ) USize[InCapacity + 1ULL]() );

    if ( not NewNodes.IsValid() or not NewPublishedTransforms.IsValid() or not NewLevelOrdered.IsValid() or
         not NewLevelStarts.IsValid() or not NewLevelCursor.IsValid() )
    {
        return MakeUnexpected( ESceneHierarchyError::OutOfMemory );
    }

    Nodes                  = std::move( NewNodes );
    PublishedTransforms    = std::move( NewPublishedTransforms );
    LevelOrderedTransforms = std::move( NewLevelOrdered );
    LevelStarts            = std::move( NewLevelStarts );
    LevelCursor            = std::move( NewLevelCursor );
    Capacity               = InCapacity;
    return {};
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError>
Lumen::FSceneHierarchy::Register ( const ActorID InActorId, const Maths::FMat4x4f &InLocalTransform ) noexcept
{
    if ( not IsInitialized() )
    {
        return MakeUnexpected( ESceneHierarchyError::NotInitialized );
    }

    if ( InActorId == InvalidActorID )
    {
        return MakeUnexpected( ESceneHierarchyError::InvalidActor );
    }

    const USize InsertionIndex = LowerBound( InActorId );
    if ( InsertionIndex < NodeCount and Nodes[InsertionIndex].Id == InActorId )
    {
        return MakeUnexpected( ESceneHierarchyError::AlreadyRegistered );
    }

    if ( NodeCount == Capacity )
    {
        return MakeUnexpected( ESceneHierarchyError::CapacityExceeded );
    }

    for ( USize Index = NodeCount; Index > InsertionIndex; --Index )
    {
        Nodes[Index] = Nodes[Index - 1ULL];
    }

    FNode NewNode;
    NewNode.Id                = InActorId;
    NewNode.LocalTransform    = InLocalTransform;
    NewNode.WorldTransform    = InLocalTransform;
    NewNode.bWorldDirty       = true;
    NewNode.bPublicationDirty = true;

    Nodes[InsertionIndex] = NewNode;
    ++NodeCount;

    LinkNode( Nodes[InsertionIndex] );
    bAnyWorldDirty       = true;
    bAnyPublicationDirty = true;
    bStructureDirty      = true;
    return {};
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError> Lumen::FSceneHierarchy::Unregister ( const ActorID InActorId ) noexcept
{
    if ( not IsInitialized() )
    {
        return MakeUnexpected( ESceneHierarchyError::NotInitialized );
    }

    if ( InActorId == InvalidActorID )
    {
        return MakeUnexpected( ESceneHierarchyError::InvalidActor );
    }

    const USize NodeIndex = FindNodeIndex( InActorId );
    if ( NodeIndex == NodeCount )
    {
        return MakeUnexpected( ESceneHierarchyError::UnknownActor );
    }

    FNode &Node = Nodes[NodeIndex];
    if ( Node.FirstChildId != InvalidActorID )
    {
        static_cast<void>( UpdateWorldTransforms() );

        ActorID ChildId = Node.FirstChildId;
        while ( ChildId != InvalidActorID )
        {
            const USize ChildIndex = FindNodeIndex( ChildId );
            LUMEN_ASSERT( ChildIndex < NodeCount );
            FNode &Child              = Nodes[ChildIndex];
            const ActorID NextChildId = Child.NextSiblingId;

            Child.LocalTransform = Child.WorldTransform;
            Child.ParentId       = InvalidActorID;
            LinkNode( Child );
            MarkDirty( Child );
            ChildId = NextChildId;
        }

        Node.FirstChildId = InvalidActorID;
        Node.LastChildId  = InvalidActorID;
    }

    UnlinkNode( Node );

    for ( USize Index = NodeIndex; Index + 1ULL < NodeCount; ++Index )
    {
        Nodes[Index] = Nodes[Index + 1ULL];
    }

    --NodeCount;
    bStructureDirty = true;
    return {};
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError> Lumen::FSceneHierarchy::Attach ( const ActorID InActorId,
                                                                                     const ActorID InParentId ) noexcept
{
    if ( not IsInitialized() )
    {
        return MakeUnexpected( ESceneHierarchyError::NotInitialized );
    }

    if ( InActorId == InvalidActorID )
    {
        return MakeUnexpected( ESceneHierarchyError::InvalidActor );
    }

    if ( InActorId == InParentId )
    {
        return MakeUnexpected( ESceneHierarchyError::SelfParent );
    }

    const USize NodeIndex = FindNodeIndex( InActorId );
    if ( NodeIndex == NodeCount )
    {
        return MakeUnexpected( ESceneHierarchyError::UnknownActor );
    }

    if ( InParentId == InvalidActorID )
    {
        return Detach( InActorId );
    }

    const USize ParentIndex = FindNodeIndex( InParentId );
    if ( ParentIndex == NodeCount )
    {
        return MakeUnexpected( ESceneHierarchyError::UnknownParent );
    }

    FNode &Node = Nodes[NodeIndex];
    if ( Node.ParentId == InParentId )
    {
        return {};
    }

    ActorID AncestorId = InParentId;
    for ( USize Step = 0ULL; Step < NodeCount and AncestorId != InvalidActorID; ++Step )
    {
        if ( AncestorId == InActorId )
        {
            return MakeUnexpected( ESceneHierarchyError::CycleDetected );
        }

        const USize AncestorIndex = FindNodeIndex( AncestorId );
        if ( AncestorIndex == NodeCount )
        {
            return MakeUnexpected( ESceneHierarchyError::UnknownParent );
        }

        AncestorId = Nodes[AncestorIndex].ParentId;
    }

    if ( AncestorId != InvalidActorID )
    {
        return MakeUnexpected( ESceneHierarchyError::CycleDetected );
    }

    UnlinkNode( Node );
    Node.ParentId = InParentId;
    LinkNode( Node );
    MarkDirty( Node );
    bStructureDirty = true;
    return {};
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError> Lumen::FSceneHierarchy::Detach ( const ActorID InActorId ) noexcept
{
    if ( not IsInitialized() )
    {
        return MakeUnexpected( ESceneHierarchyError::NotInitialized );
    }

    if ( InActorId == InvalidActorID )
    {
        return MakeUnexpected( ESceneHierarchyError::InvalidActor );
    }

    const USize NodeIndex = FindNodeIndex( InActorId );
    if ( NodeIndex == NodeCount )
    {
        return MakeUnexpected( ESceneHierarchyError::UnknownActor );
    }

    static_cast<void>( UpdateWorldTransforms() );

    FNode &Node = Nodes[NodeIndex];
    if ( Node.ParentId == InvalidActorID )
    {
        Node.LocalTransform = Node.WorldTransform;
        return {};
    }

    UnlinkNode( Node );
    Node.LocalTransform = Node.WorldTransform;
    LinkNode( Node );
    MarkDirty( Node );
    bStructureDirty = true;
    return {};
}

Lumen::TExpected<void, Lumen::ESceneHierarchyError>
Lumen::FSceneHierarchy::SetLocalTransform ( const ActorID InActorId, const Maths::FMat4x4f &InLocalTransform ) noexcept
{
    if ( not IsInitialized() )
    {
        return MakeUnexpected( ESceneHierarchyError::NotInitialized );
    }

    if ( InActorId == InvalidActorID )
    {
        return MakeUnexpected( ESceneHierarchyError::InvalidActor );
    }

    const USize NodeIndex = FindNodeIndex( InActorId );
    if ( NodeIndex == NodeCount )
    {
        return MakeUnexpected( ESceneHierarchyError::UnknownActor );
    }

    FNode &Node = Nodes[NodeIndex];
    if ( Node.LocalTransform == InLocalTransform )
    {
        return {};
    }

    Node.LocalTransform = InLocalTransform;
    MarkDirty( Node );
    return {};
}

Lumen::TExpected<Lumen::FSceneHierarchyUpdateResult, Lumen::ESceneHierarchyError> Lumen::FSceneHierarchy::UpdateWorld () noexcept
{
    if ( not IsInitialized() )
    {
        return MakeUnexpected( ESceneHierarchyError::NotInitialized );
    }

    FSceneHierarchyUpdateResult Result;
    Result.UpdatedTransformCount = UpdateWorldTransforms();

    if ( bStructureDirty )
    {
        for ( USize Index = 0ULL; Index < NodeCount; ++Index )
        {
            PublishedTransforms[Index].ActorId        = Nodes[Index].Id;
            PublishedTransforms[Index].WorldTransform = Nodes[Index].WorldTransform;
            Nodes[Index].bPublicationDirty            = false;
        }

        PublishedTransformCount = NodeCount;
        bStructureDirty         = false;
    }
    else if ( bAnyPublicationDirty )
    {
        LUMEN_ASSERT( PublishedTransformCount == NodeCount );
        for ( USize Index = 0ULL; Index < NodeCount; ++Index )
        {
            if ( Nodes[Index].bPublicationDirty )
            {
                PublishedTransforms[Index].WorldTransform = Nodes[Index].WorldTransform;
                Nodes[Index].bPublicationDirty            = false;
            }
        }
    }

    bAnyPublicationDirty = false;
    ++PublicationSequence;
    RebuildLevelOrder();
    Result.PublishedTransformCount = PublishedTransformCount;
    Result.PublicationSequence     = PublicationSequence;
    return Result;
}

Lumen::TExpected<Lumen::FSceneHierarchyUpdateResult, Lumen::ESceneHierarchyError> Lumen::FSceneHierarchy::Publish () noexcept
{
    return UpdateWorld();
}

Lumen::TOptional<Lumen::Maths::FMat4x4f> Lumen::FSceneHierarchy::GetWorldTransform ( const ActorID InActorId ) const noexcept
{
    const USize NodeIndex = FindNodeIndex( InActorId );
    if ( NodeIndex == NodeCount )
    {
        return {};
    }

    return TOptional<Maths::FMat4x4f>( Nodes[NodeIndex].WorldTransform );
}

Lumen::TOptional<Lumen::Maths::FMat4x4f> Lumen::FSceneHierarchy::GetLocalTransform ( const ActorID InActorId ) const noexcept
{
    const USize NodeIndex = FindNodeIndex( InActorId );
    if ( NodeIndex == NodeCount )
    {
        return {};
    }

    return TOptional<Maths::FMat4x4f>( Nodes[NodeIndex].LocalTransform );
}

Lumen::TOptional<Lumen::ActorID> Lumen::FSceneHierarchy::GetParentId ( const ActorID InActorId ) const noexcept
{
    const USize NodeIndex = FindNodeIndex( InActorId );
    if ( NodeIndex == NodeCount )
    {
        return {};
    }

    return TOptional<ActorID>( Nodes[NodeIndex].ParentId );
}

Lumen::Bool Lumen::FSceneHierarchy::Contains ( const ActorID InActorId ) const noexcept
{
    return FindNodeIndex( InActorId ) != NodeCount;
}

Lumen::Bool Lumen::FSceneHierarchy::IsInitialized () const noexcept
{
    return Capacity != 0ULL;
}

Lumen::USize Lumen::FSceneHierarchy::GetCapacity () const noexcept
{
    return Capacity;
}

Lumen::USize Lumen::FSceneHierarchy::GetNodeCount () const noexcept
{
    return NodeCount;
}

Lumen::UInt64 Lumen::FSceneHierarchy::GetPublicationSequence () const noexcept
{
    return PublicationSequence;
}

Lumen::TSpan<const Lumen::FSceneHierarchyTransformRecord> Lumen::FSceneHierarchy::GetPublishedTransforms () const noexcept
{
    if ( PublishedTransformCount == 0ULL )
    {
        return {};
    }

    return { PublishedTransforms.Get(), PublishedTransformCount };
}

Lumen::USize Lumen::FSceneHierarchy::GetRegisteredActorIds ( const TSpan<ActorID> OutActorIds ) const noexcept
{
    USize Written = 0ULL;
    for ( USize Index = 0ULL; Index < NodeCount and Written < OutActorIds.size(); ++Index )
    {
        OutActorIds[Written++] = Nodes[Index].Id;
    }
    return Written;
}

Lumen::TOptional<Lumen::USize> Lumen::FSceneHierarchy::GetDepth ( const ActorID InActorId ) const noexcept
{
    const USize NodeIndex = FindNodeIndex( InActorId );
    if ( NodeIndex == NodeCount )
    {
        return {};
    }

    return TOptional<USize>( Nodes[NodeIndex].Depth );
}

Lumen::TSpan<const Lumen::USize> Lumen::FSceneHierarchy::GetLevelStarts () const noexcept
{
    if ( not LevelStarts.IsValid() )
    {
        return {};
    }

    return TSpan<const USize>( LevelStarts.Get(), Capacity + 1ULL );
}

Lumen::TSpan<const Lumen::FSceneHierarchyTransformRecord> Lumen::FSceneHierarchy::GetLevelOrderedTransforms () const noexcept
{
    if ( not LevelOrderedTransforms.IsValid() or PublishedTransformCount == 0ULL )
    {
        return {};
    }

    return { LevelOrderedTransforms.Get(), PublishedTransformCount };
}

void Lumen::FSceneHierarchy::RebuildLevelOrder () noexcept
{
    for ( USize Index = 0ULL; Index <= Capacity; ++Index )
    {
        LevelStarts[Index] = 0ULL;
    }

    for ( USize Index = 0ULL; Index < PublishedTransformCount; ++Index )
    {
        ++LevelStarts[Nodes[Index].Depth + 1ULL];
    }

    for ( USize Depth = 0ULL; Depth < Capacity; ++Depth )
    {
        LevelStarts[Depth + 1ULL] += LevelStarts[Depth];
    }

    for ( USize Index = 0ULL; Index <= Capacity; ++Index )
    {
        LevelCursor[Index] = LevelStarts[Index];
    }

    for ( USize Index = 0ULL; Index < PublishedTransformCount; ++Index )
    {
        LevelOrderedTransforms[LevelCursor[Nodes[Index].Depth]++] = PublishedTransforms[Index];
    }
}

Lumen::USize Lumen::FSceneHierarchy::LowerBound ( const ActorID InActorId ) const noexcept
{
    USize First = 0ULL;
    USize Last  = NodeCount;

    while ( First < Last )
    {
        const USize Middle = First + ( ( Last - First ) / 2ULL );
        if ( Nodes[Middle].Id < InActorId )
        {
            First = Middle + 1ULL;
        }
        else
        {
            Last = Middle;
        }
    }

    return First;
}

Lumen::USize Lumen::FSceneHierarchy::FindNodeIndex ( const ActorID InActorId ) const noexcept
{
    if ( InActorId == InvalidActorID )
    {
        return NodeCount;
    }

    const USize NodeIndex = LowerBound( InActorId );
    return NodeIndex < NodeCount and Nodes[NodeIndex].Id == InActorId ? NodeIndex : NodeCount;
}

void Lumen::FSceneHierarchy::LinkNode ( FNode &InNode ) noexcept
{
    ActorID *FirstChildId = &RootFirstChildId;
    ActorID *LastChildId  = &RootLastChildId;

    if ( InNode.ParentId != InvalidActorID )
    {
        const USize ParentIndex = FindNodeIndex( InNode.ParentId );
        LUMEN_ASSERT( ParentIndex < NodeCount );
        FNode &Parent = Nodes[ParentIndex];
        FirstChildId  = &Parent.FirstChildId;
        LastChildId   = &Parent.LastChildId;
    }

    InNode.PreviousSiblingId = InvalidActorID;
    InNode.NextSiblingId     = InvalidActorID;

    if ( *FirstChildId == InvalidActorID )
    {
        *FirstChildId = InNode.Id;
        *LastChildId  = InNode.Id;
        return;
    }

    if ( InNode.Id < *FirstChildId )
    {
        const USize FirstIndex = FindNodeIndex( *FirstChildId );
        LUMEN_ASSERT( FirstIndex < NodeCount );
        Nodes[FirstIndex].PreviousSiblingId = InNode.Id;
        InNode.NextSiblingId                = *FirstChildId;
        *FirstChildId                       = InNode.Id;
        return;
    }

    if ( InNode.Id > *LastChildId )
    {
        const USize LastIndex = FindNodeIndex( *LastChildId );
        LUMEN_ASSERT( LastIndex < NodeCount );
        Nodes[LastIndex].NextSiblingId = InNode.Id;
        InNode.PreviousSiblingId       = *LastChildId;
        *LastChildId                   = InNode.Id;
        return;
    }

    ActorID CursorId = *FirstChildId;
    for ( USize Step = 0ULL; Step < NodeCount and CursorId != InvalidActorID; ++Step )
    {
        const USize CursorIndex = FindNodeIndex( CursorId );
        LUMEN_ASSERT( CursorIndex < NodeCount );
        FNode &Cursor = Nodes[CursorIndex];

        if ( InNode.Id < Cursor.Id )
        {
            const ActorID PreviousId  = Cursor.PreviousSiblingId;
            const USize PreviousIndex = FindNodeIndex( PreviousId );
            LUMEN_ASSERT( PreviousIndex < NodeCount );

            InNode.PreviousSiblingId           = PreviousId;
            InNode.NextSiblingId               = Cursor.Id;
            Nodes[PreviousIndex].NextSiblingId = InNode.Id;
            Cursor.PreviousSiblingId           = InNode.Id;
            return;
        }

        CursorId = Cursor.NextSiblingId;
    }

    LUMEN_ASSERT( false );
}

void Lumen::FSceneHierarchy::UnlinkNode ( FNode &InNode ) noexcept
{
    ActorID *FirstChildId = &RootFirstChildId;
    ActorID *LastChildId  = &RootLastChildId;

    if ( InNode.ParentId != InvalidActorID )
    {
        const USize ParentIndex = FindNodeIndex( InNode.ParentId );
        LUMEN_ASSERT( ParentIndex < NodeCount );
        FNode &Parent = Nodes[ParentIndex];
        FirstChildId  = &Parent.FirstChildId;
        LastChildId   = &Parent.LastChildId;
    }

    if ( InNode.PreviousSiblingId == InvalidActorID )
    {
        *FirstChildId = InNode.NextSiblingId;
    }
    else
    {
        const USize PreviousIndex = FindNodeIndex( InNode.PreviousSiblingId );
        LUMEN_ASSERT( PreviousIndex < NodeCount );
        Nodes[PreviousIndex].NextSiblingId = InNode.NextSiblingId;
    }

    if ( InNode.NextSiblingId == InvalidActorID )
    {
        *LastChildId = InNode.PreviousSiblingId;
    }
    else
    {
        const USize NextIndex = FindNodeIndex( InNode.NextSiblingId );
        LUMEN_ASSERT( NextIndex < NodeCount );
        Nodes[NextIndex].PreviousSiblingId = InNode.PreviousSiblingId;
    }

    InNode.ParentId          = InvalidActorID;
    InNode.PreviousSiblingId = InvalidActorID;
    InNode.NextSiblingId     = InvalidActorID;
}

void Lumen::FSceneHierarchy::MarkDirty ( FNode &InNode ) noexcept
{
    InNode.bWorldDirty = true;
    bAnyWorldDirty     = true;
}

Lumen::USize Lumen::FSceneHierarchy::UpdateWorldTransforms () noexcept
{
    if ( not bAnyWorldDirty )
    {
        return 0ULL;
    }

    USize UpdatedCount = 0ULL;
    USize VisitedCount = 0ULL;
    ActorID CurrentId  = RootFirstChildId;

    while ( CurrentId != InvalidActorID and VisitedCount < NodeCount )
    {
        const USize CurrentIndex = FindNodeIndex( CurrentId );
        LUMEN_ASSERT( CurrentIndex < NodeCount );
        FNode &Node = Nodes[CurrentIndex];

        FNode *Parent = nullptr;
        if ( Node.ParentId != InvalidActorID )
        {
            const USize ParentIndex = FindNodeIndex( Node.ParentId );
            LUMEN_ASSERT( ParentIndex < NodeCount );
            Parent = &Nodes[ParentIndex];
        }

        const Bool bShouldUpdate = Node.bWorldDirty or ( Parent != nullptr and Parent->bWorldUpdated );
        Node.bWorldUpdated       = bShouldUpdate;

        if ( bShouldUpdate )
        {
            if ( Parent == nullptr )
            {
                Node.WorldTransform = Node.LocalTransform;
                Node.Depth          = 0ULL;
            }
            else
            {
                LUMEN_ASSERT( Parent->Depth + 1ULL < Capacity );
                Node.WorldTransform = Parent->WorldTransform * Node.LocalTransform;
                Node.Depth          = Parent->Depth + 1ULL;
            }

            Node.bWorldDirty       = false;
            Node.bPublicationDirty = true;
            bAnyPublicationDirty   = true;
            ++UpdatedCount;
        }

        ++VisitedCount;
        if ( Node.FirstChildId != InvalidActorID )
        {
            CurrentId = Node.FirstChildId;
            continue;
        }

        ActorID CursorId = CurrentId;
        CurrentId        = InvalidActorID;
        while ( CursorId != InvalidActorID )
        {
            const USize CursorIndex = FindNodeIndex( CursorId );
            LUMEN_ASSERT( CursorIndex < NodeCount );
            const FNode &Cursor = Nodes[CursorIndex];
            if ( Cursor.NextSiblingId != InvalidActorID )
            {
                CurrentId = Cursor.NextSiblingId;
                break;
            }

            CursorId = Cursor.ParentId;
        }
    }

    LUMEN_ASSERT( VisitedCount == NodeCount );
    for ( USize Index = 0ULL; Index < NodeCount; ++Index )
    {
        Nodes[Index].bWorldUpdated = false;
    }

    bAnyWorldDirty = false;
    return UpdatedCount;
}
