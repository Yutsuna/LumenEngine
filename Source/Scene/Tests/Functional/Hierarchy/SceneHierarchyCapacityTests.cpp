/**
 * @file SceneHierarchyCapacityTests.cpp
 * @brief Capacity, depth and allocation contracts of the bounded actor hierarchy.
 */

#include <Hierarchy/SceneHierarchy.hpp>

#include <Actor/ActorTypes.hpp>

#include <AllocationTracking.hpp>
#include <Templates/Array.hpp>
#include <Templates/Vector.hpp>
#include <Tests/UnitTest.hpp>

#include <bit>

namespace Lumen
{

namespace
{

    [[nodiscard]] ActorID CapacityActor ( const UInt32 InSlot ) noexcept
    {
        return MakeActorId( InSlot, 7U );
    }

    [[nodiscard]] UInt64 DigestPublished ( const FSceneHierarchy &InHierarchy ) noexcept
    {
        UInt64 Digest = 14695981039346656037ULL;
        for ( const FSceneHierarchyTransformRecord &Record : InHierarchy.GetPublishedTransforms() )
        {
            const UInt64 Id = static_cast<UInt64>( Record.ActorId );
            for ( USize ByteIndex = 0ULL; ByteIndex < 8ULL; ++ByteIndex )
            {
                Digest ^= ( Id >> ( ByteIndex * 8ULL ) ) & 0xFFULL;
                Digest *= 1099511628211ULL;
            }

            const TArray<UInt32, 16ULL> Words = std::bit_cast<TArray<UInt32, 16ULL>>( Record.WorldTransform );
            for ( const UInt32 Word : Words )
            {
                for ( USize ByteIndex = 0ULL; ByteIndex < 4ULL; ++ByteIndex )
                {
                    Digest ^= ( static_cast<UInt64>( Word ) >> ( ByteIndex * 8ULL ) ) & 0xFFULL;
                    Digest *= 1099511628211ULL;
                }
            }
        }
        return Digest;
    }

    LUMEN_TEST( FSceneHierarchyCapacity, CapacityPlusOneIsRejectedWithoutMutatingExistingNodes )
    {
        FSceneHierarchy Hierarchy;
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Initialize( 3ULL ) );

        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( CapacityActor( 1U ) ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( CapacityActor( 2U ) ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( CapacityActor( 3U ) ) );

        const TExpected<void, ESceneHierarchyError> Overflow = Hierarchy.Register( CapacityActor( 4U ) );
        LUMEN_CHECK_HAS_ERROR( Overflow );
        LUMEN_CHECK_TRUE( Overflow.Error() == ESceneHierarchyError::CapacityExceeded );

        LUMEN_CHECK_EQ( Hierarchy.GetNodeCount(), 3ULL );
        LUMEN_CHECK_FALSE( Hierarchy.Contains( CapacityActor( 4U ) ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.UpdateWorld() );
        LUMEN_CHECK_EQ( Hierarchy.GetPublishedTransforms().size(), 3ULL );
    }

    LUMEN_TEST( FSceneHierarchyCapacity, FullDepthChainIsValidAndLevelIterationMatchesDepths )
    {
        constexpr USize NodeCount = 100ULL;

        FSceneHierarchy Hierarchy;
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Initialize( NodeCount ) );

        USize Visited = 0ULL;
        for ( USize Index = 0ULL; Index < NodeCount; ++Index )
        {
            LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( CapacityActor( static_cast<UInt32>( Index ) + 1U ) ) );
            if ( Index > 0ULL )
            {
                LUMEN_CHECK_HAS_VALUE( Hierarchy.Attach( CapacityActor( static_cast<UInt32>( Index ) + 1U ),
                                                         CapacityActor( static_cast<UInt32>( Index ) ) ) );
            }
        }

        LUMEN_CHECK_HAS_VALUE( Hierarchy.UpdateWorld() );
        LUMEN_CHECK_EQ( Hierarchy.GetDepth( CapacityActor( static_cast<UInt32>( NodeCount ) ) ).ValueOr( 0ULL ),
                        NodeCount - 1ULL );
        LUMEN_CHECK_EQ( Hierarchy.GetLevelStarts()[NodeCount], NodeCount );

        for ( USize Depth = 0ULL; Depth < NodeCount; ++Depth )
        {
            Hierarchy.ForEachLevel( Depth, [&Visited] ( const FSceneHierarchyTransformRecord & ) noexcept
            {
                ++Visited;
            } );
        }
        LUMEN_CHECK_EQ( Visited, NodeCount );
    }

    LUMEN_TEST( FSceneHierarchyCapacity, HotMutationsAndPublicationDoNotAllocate )
    {
        FSceneHierarchy Hierarchy;
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Initialize( 16ULL ) );

        const ActorID Root  = CapacityActor( 1U );
        const ActorID Child = CapacityActor( 2U );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( Root ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( Child ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.Attach( Child, Root ) );
        LUMEN_CHECK_HAS_VALUE( Hierarchy.UpdateWorld() );

        const FAllocationTrackingScope Tracking;
        for ( USize Index = 0ULL; Index < 1024ULL; ++Index )
        {
            const Maths::FMat4x4f Local =
                Maths::FMat4x4f::Translate( Maths::FVec3f{ static_cast<Float32>( Index ), 0.0F, 0.0F } );
            LUMEN_CHECK_HAS_VALUE( Hierarchy.SetLocalTransform( Child, Local ) );
            LUMEN_CHECK_HAS_VALUE( Hierarchy.UpdateWorld() );
            LUMEN_CHECK_EQ( Hierarchy.GetPublishedTransforms().size(), 2ULL );
        }
        LUMEN_CHECK_EQ( Tracking.GetCount(), 0ULL );
    }

    LUMEN_TEST( FSceneHierarchyCapacity, CanonicalDigestIsIndependentOfMutationOrder )
    {
        constexpr UInt32 ActorCount = 8U;
        constexpr UInt32 Runs       = 32U;

        UInt64 Canonical = 0ULL;
        for ( UInt32 RunIndex = 0U; RunIndex < Runs; ++RunIndex )
        {
            FSceneHierarchy Hierarchy;
            LUMEN_CHECK_HAS_VALUE( Hierarchy.Initialize( ActorCount ) );

            /** INFO: Rotation of a fixed ID set with identical local transforms; topology is always the same star. */
            LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( CapacityActor( 1U ) ) );
            for ( UInt32 Offset = 0U; Offset < ActorCount - 1U; ++Offset )
            {
                const UInt32 Slot = ( ( Offset + RunIndex ) % ( ActorCount - 1U ) ) + 2U;
                LUMEN_CHECK_HAS_VALUE( Hierarchy.Register( CapacityActor( Slot ) ) );
                LUMEN_CHECK_HAS_VALUE( Hierarchy.Attach( CapacityActor( Slot ), CapacityActor( 1U ) ) );
            }

            LUMEN_CHECK_HAS_VALUE( Hierarchy.UpdateWorld() );
            const UInt64 Digest = DigestPublished( Hierarchy );
            if ( RunIndex == 0U )
            {
                Canonical = Digest;
            }
            LUMEN_CHECK_EQ( Digest, Canonical );
        }
    }

} // namespace
} // namespace Lumen
