/**
 * @file SceneHierarchyTests.cpp
 * @brief Functional contracts of the bounded actor hierarchy.
 */

#include <Hierarchy/SceneHierarchy.hpp>

#include <Actor/ActorTypes.hpp>

#include <Templates/Vector.hpp>
#include <Tests/UnitTest.hpp>

#include <array>
#include <bit>

namespace Lumen
{

namespace
{

    [[nodiscard]] ActorID HierarchyActor ( const UInt32 InSlot ) noexcept
    {
        return MakeActorId( InSlot, 1U );
    }

    [[nodiscard]] std::array<UInt32, 16ULL> MatrixBits ( const Maths::FMat4x4f &InMatrix ) noexcept
    {
        return std::bit_cast<std::array<UInt32, 16ULL>>( InMatrix );
    }

    LUMEN_TEST( FSceneHierarchy, InitializeRejectsInvalidCapacityAndDoubleInitialization )
    {
        FSceneHierarchy Hierarchy;

        const TExpected<void, ESceneHierarchyError> Zero = Hierarchy.Initialize( 0ULL );
        LUMEN_CHECK_HAS_ERROR( Zero );
        LUMEN_CHECK_TRUE( Zero.Error() == ESceneHierarchyError::InvalidCapacity );

        const TExpected<void, ESceneHierarchyError> BeforeInit = Hierarchy.Register( HierarchyActor( 1U ) );
        LUMEN_CHECK_HAS_ERROR( BeforeInit );
        LUMEN_CHECK_TRUE( BeforeInit.Error() == ESceneHierarchyError::NotInitialized );

        LUMEN_CHECK_HAS_VALUE( Hierarchy.Initialize( 4ULL ) );
        LUMEN_CHECK_TRUE( Hierarchy.IsInitialized() );
        LUMEN_CHECK_EQ( Hierarchy.GetCapacity(), 4ULL );
        LUMEN_CHECK_EQ( Hierarchy.GetNodeCount(), 0ULL );

        const TExpected<void, ESceneHierarchyError> Again = Hierarchy.Initialize( 8ULL );
        LUMEN_CHECK_HAS_ERROR( Again );
        LUMEN_CHECK_TRUE( Again.Error() == ESceneHierarchyError::AlreadyInitialized );
        LUMEN_CHECK_EQ( Hierarchy.GetCapacity(), 4ULL );
    }

    LUMEN_TEST( FSceneHierarchy, RegisterRejectsDuplicatesAndAttachRejectsUnknownWeakParents )
    {
        FSceneHierarchy Hierarchy;
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Initialize( 8ULL ) );

        const ActorID Root    = HierarchyActor( 1U );
        const ActorID Child   = HierarchyActor( 2U );
        const ActorID Outside = HierarchyActor( 9U );

        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( Root ) );
        const TExpected<void, ESceneHierarchyError> Duplicate = Hierarchy.Register( Root );
        LUMEN_CHECK_HAS_ERROR( Duplicate );
        LUMEN_CHECK_TRUE( Duplicate.Error() == ESceneHierarchyError::AlreadyRegistered );

        const TExpected<void, ESceneHierarchyError> Self = Hierarchy.Attach( Root, Root );
        LUMEN_CHECK_HAS_ERROR( Self );
        LUMEN_CHECK_TRUE( Self.Error() == ESceneHierarchyError::SelfParent );

        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( Child ) );
        const TExpected<void, ESceneHierarchyError> Unknown = Hierarchy.Attach( Child, Outside );
        LUMEN_CHECK_HAS_ERROR( Unknown );
        LUMEN_CHECK_TRUE( Unknown.Error() == ESceneHierarchyError::UnknownParent );

        const TOptional<ActorID> Parent = Hierarchy.GetParentId( Root );
        LUMEN_CHECK_TRUE( Parent.HasValue() );
        LUMEN_CHECK_EQ( Parent.ValueOr( InvalidActorID ), InvalidActorID );
        LUMEN_CHECK_TRUE( Hierarchy.Contains( Child ) );
    }

    LUMEN_TEST( FSceneHierarchy, AttachRejectsCyclesWithoutMutatingTopologyOrTransform )
    {
        FSceneHierarchy Hierarchy;
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Initialize( 8ULL ) );

        const ActorID Root       = HierarchyActor( 1U );
        const ActorID Child      = HierarchyActor( 2U );
        const ActorID GrandChild = HierarchyActor( 3U );

        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( Root ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( Child, Maths::FMat4x4f::Translate( Maths::FVec3f{ 2.0F, 0.0F, 0.0F } ) ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( GrandChild ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Attach( Child, Root ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Attach( GrandChild, Child ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.UpdateWorld() );

        const TOptional<ActorID> RootParentBefore    = Hierarchy.GetParentId( Root );
        const TOptional<Maths::FMat4x4f> WorldBefore = Hierarchy.GetWorldTransform( Root );
        const std::array<UInt32, 16ULL> BitsBefore   = MatrixBits( WorldBefore.ValueOr( Maths::FMat4x4f::Identity() ) );

        const TExpected<void, ESceneHierarchyError> Cycle = Hierarchy.Attach( Root, GrandChild );
        LUMEN_CHECK_HAS_ERROR( Cycle );
        LUMEN_CHECK_TRUE( Cycle.Error() == ESceneHierarchyError::CycleDetected );

        const TOptional<ActorID> RootParentAfter    = Hierarchy.GetParentId( Root );
        const TOptional<Maths::FMat4x4f> WorldAfter = Hierarchy.GetWorldTransform( Root );
        LUMEN_CHECK_EQ( RootParentBefore.ValueOr( InvalidActorID ), RootParentAfter.ValueOr( InvalidActorID ) );
        LUMEN_CHECK_TRUE( MatrixBits( WorldAfter.ValueOr( Maths::FMat4x4f::Identity() ) ) == BitsBefore );
        LUMEN_CHECK_EQ( Hierarchy.GetDepth( Root ).ValueOr( 99ULL ), 0ULL );
        LUMEN_CHECK_EQ( Hierarchy.GetDepth( Child ).ValueOr( 99ULL ), 1ULL );
        LUMEN_CHECK_EQ( Hierarchy.GetDepth( GrandChild ).ValueOr( 99ULL ), 2ULL );
    }

    LUMEN_TEST( FSceneHierarchy, DetachAndUnregisterPreserveWorldTransformForChildren )
    {
        FSceneHierarchy Hierarchy;
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Initialize( 8ULL ) );

        const ActorID Root       = HierarchyActor( 1U );
        const ActorID Child      = HierarchyActor( 2U );
        const ActorID GrandChild = HierarchyActor( 3U );

        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( Root, Maths::FMat4x4f::Translate( Maths::FVec3f{ 10.0F, 0.0F, 0.0F } ) ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( Child, Maths::FMat4x4f::Translate( Maths::FVec3f{ 1.0F, 0.0F, 0.0F } ) ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( GrandChild ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Attach( Child, Root ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Attach( GrandChild, Child ) );

        /** INFO: Detach resolves pending dirty transforms before changing the parent; capture the published world first. */
        LUMEN_CHECK_HAS_VALUE( Hierarchy.UpdateWorld() );
        const TOptional<Maths::FMat4x4f> ExpectedChildWorld = Hierarchy.GetWorldTransform( Child );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Detach( Child ) );
        const TOptional<Maths::FMat4x4f> ChildLocal = Hierarchy.GetLocalTransform( Child );
        LUMEN_CHECK_TRUE( MatrixBits( ChildLocal.ValueOr( Maths::FMat4x4f::Identity() ) ) ==
                          MatrixBits( ExpectedChildWorld.ValueOr( Maths::FMat4x4f::Identity() ) ) );
        LUMEN_CHECK_EQ( Hierarchy.GetParentId( Child ).ValueOr( InvalidActorID ), InvalidActorID );

        LUMEN_CHECK_HAS_VALUE( Hierarchy.Attach( Child, Root ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.UpdateWorld() );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Unregister( Root ) );
        LUMEN_CHECK_FALSE( Hierarchy.Contains( Root ) );
        LUMEN_CHECK_EQ( Hierarchy.GetParentId( Child ).ValueOr( InvalidActorID ), InvalidActorID );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.UpdateWorld() );
        LUMEN_CHECK_EQ( Hierarchy.GetDepth( Child ).ValueOr( 99ULL ), 0ULL );
    }

    LUMEN_TEST( FSceneHierarchy, PublishedAndLevelOrderedViewsAreCanonical )
    {
        FSceneHierarchy Hierarchy;
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Initialize( 8ULL ) );

        const ActorID LowId  = HierarchyActor( 1U );
        const ActorID MidId  = HierarchyActor( 5U );
        const ActorID HighId = HierarchyActor( 9U );

        /** INFO: Reverse registration order must not leak into either published view. */
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( HighId ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( MidId ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( LowId ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Attach( HighId, LowId ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Attach( MidId, LowId ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.UpdateWorld() );

        const TSpan<const FSceneHierarchyTransformRecord> Published = Hierarchy.GetPublishedTransforms();
        LUMEN_CHECK_EQ( Published.size(), 3ULL );
        LUMEN_CHECK_TRUE( Published[0ULL].ActorId < Published[1ULL].ActorId );
        LUMEN_CHECK_TRUE( Published[1ULL].ActorId < Published[2ULL].ActorId );

        const TSpan<const FSceneHierarchyTransformRecord> LevelOrdered = Hierarchy.GetLevelOrderedTransforms();
        LUMEN_CHECK_EQ( LevelOrdered.size(), 3ULL );
        LUMEN_CHECK_EQ( LevelOrdered[0ULL].ActorId, LowId );
        LUMEN_CHECK_EQ( LevelOrdered[1ULL].ActorId, MidId );
        LUMEN_CHECK_EQ( LevelOrdered[2ULL].ActorId, HighId );

        const TSpan<const USize> Starts = Hierarchy.GetLevelStarts();
        LUMEN_CHECK_EQ( Starts.size(), 9ULL );
        LUMEN_CHECK_EQ( Starts[0ULL], 0ULL );
        LUMEN_CHECK_EQ( Starts[1ULL], 1ULL );
        LUMEN_CHECK_EQ( Starts[2ULL], 3ULL );

        TVector<ActorID> VisitedIds;
        VisitedIds.reserve( 2ULL );
        Hierarchy.ForEachLevel( 1ULL, [&VisitedIds] ( const FSceneHierarchyTransformRecord &InRecord ) noexcept
        {
            VisitedIds.push_back( InRecord.ActorId );
        } );
        LUMEN_CHECK_EQ( VisitedIds.size(), 2ULL );
        LUMEN_CHECK_TRUE( VisitedIds[0ULL] == MidId or VisitedIds[0ULL] == HighId );
        LUMEN_CHECK_TRUE( VisitedIds[1ULL] == MidId or VisitedIds[1ULL] == HighId );
    }

} // namespace
} // namespace Lumen
