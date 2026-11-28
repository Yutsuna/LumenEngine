/**
 * @file HierarchyDeferredCommandTests.cpp
 * @brief Deterministic deferred hierarchy commands issued from actor phases.
 */

#include <World/World.hpp>
#include <World/WorldTypes.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorTypes.hpp>
#include <Hierarchy/SceneHierarchy.hpp>

#include <Templates/Array.hpp>
#include <Templates/Vector.hpp>
#include <Tests/UnitTest.hpp>

#include <bit>

namespace Lumen
{

namespace
{

    [[nodiscard]] FWorldDesc MakeHierarchyDesc ( const UInt32 InWorkerCount, const USize InCommandCapacity ) noexcept
    {
        FWorldDesc Desc;
        Desc.WorkerCount             = InWorkerCount;
        Desc.MaxActors               = 16ULL;
        Desc.HierarchyCapacity       = 16ULL;
        Desc.DeferredCommandCapacity = InCommandCapacity;
        Desc.WorldId                 = 88001ULL;
        return Desc;
    }

    /** @class FHierarchyCommandActor
     * @brief Issues one hierarchy operation per tick from inside the actor phase.
     */
    class FHierarchyCommandActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FHierarchyCommandActor );

    public:

        FHierarchyCommandActor (
            const ActorID InId, FWorld *InWorld, const ActorID InChild, const ActorID InParent, const UInt32 InKind ) noexcept
            : AActor( InId ), World( InWorld ), Child( InChild ), Parent( InParent ), Kind( InKind )
        {
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            static_cast<void>( InMessage );
        }

        void OnTick ( const Float64 InDeltaTime ) noexcept override
        {
            static_cast<void>( InDeltaTime );
            TExpected<void, ESceneHierarchyError> Result;
            if ( Kind == 0U )
            {
                Result = World->TryHierarchyAttach( Child, Parent );
            }
            else if ( Kind == 1U )
            {
                Result = World->TryHierarchyAttach( Child, InvalidActorID );
            }
            else
            {
                Result = World->TrySetHierarchyLocalTransform( Child,
                                                               Maths::FMat4x4f::Translate( Maths::FVec3f{ 3.0F, 0.0F, 0.0F } ) );
            }

            if ( not Result )
            {
                LastError.store( static_cast<UInt32>( Result.Error() ), std::memory_order_release );
            }
            ++IssuedCount;
        }

        [[nodiscard]] UInt32 GetLastError () const noexcept
        {
            return LastError.load( std::memory_order_acquire );
        }

        [[nodiscard]] UInt64 GetIssuedCount () const noexcept
        {
            return IssuedCount.load( std::memory_order_relaxed );
        }

    private:

        FWorld *World  = nullptr;
        ActorID Child  = InvalidActorID;
        ActorID Parent = InvalidActorID;
        UInt32 Kind    = 0U;

        TAtomic<UInt32> LastError   = { 0U };
        TAtomic<UInt64> IssuedCount = { 0ULL };
    };

    [[nodiscard]] UInt64 HierarchyDigest ( FWorld &InWorld ) noexcept
    {
        UInt64 Digest                                             = 14695981039346656037ULL;
        const FSceneHierarchy &Hierarchy                          = InWorld.GetHierarchy();
        const TSpan<const FSceneHierarchyTransformRecord> Records = Hierarchy.GetPublishedTransforms();

        /** INFO: ActorIDs are process-wide generational slots; hash canonical ranks so separate worlds stay comparable. */
        for ( USize Index = 0ULL; Index < Records.size(); ++Index )
        {
            Digest ^= static_cast<UInt64>( Index );
            Digest *= 1099511628211ULL;

            const TArray<UInt32, 16ULL> Words = std::bit_cast<TArray<UInt32, 16ULL>>( Records[Index].WorldTransform );
            for ( const UInt32 Word : Words )
            {
                Digest ^= static_cast<UInt64>( Word );
                Digest *= 1099511628211ULL;
            }

            USize ParentRank                = Records.size();
            const TOptional<ActorID> Parent = Hierarchy.GetParentId( Records[Index].ActorId );
            if ( Parent.HasValue() )
            {
                for ( USize Candidate = 0ULL; Candidate < Records.size(); ++Candidate )
                {
                    if ( Records[Candidate].ActorId == Parent.ValueOr( InvalidActorID ) )
                    {
                        ParentRank = Candidate;
                        break;
                    }
                }
            }
            Digest ^= static_cast<UInt64>( ParentRank );
            Digest *= 1099511628211ULL;
        }
        return Digest;
    }

    LUMEN_TEST( FHierarchyDeferredCommands, AttachIssuedInsidePhaseIsAppliedOnTheNextTick )
    {
        FWorld World( MakeHierarchyDesc( 1U, 16ULL ) );

        auto ChildResult  = World.TrySpawnActor<FHierarchyCommandActor>( &World, InvalidActorID, InvalidActorID, 99U );
        auto ParentResult = World.TrySpawnActor<FHierarchyCommandActor>( &World, InvalidActorID, InvalidActorID, 99U );
        LUMEN_CHECK_HAS_VALUE( ChildResult );
        LUMEN_CHECK_HAS_VALUE( ParentResult );

        const ActorID ChildId  = ( *ChildResult )->GetId();
        const ActorID ParentId = ( *ParentResult )->GetId();
        LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ChildId ) );
        LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ParentId ) );

        auto Commander = World.TrySpawnActor<FHierarchyCommandActor>( &World, ChildId, ParentId, 0U );
        LUMEN_CHECK_HAS_VALUE( Commander );

        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_EQ( ( *Commander )->GetIssuedCount(), 1ULL );
        LUMEN_CHECK_EQ( World.GetHierarchy().GetParentId( ChildId ).ValueOr( InvalidActorID ), InvalidActorID );

        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_EQ( World.GetHierarchy().GetParentId( ChildId ).ValueOr( InvalidActorID ), ParentId );
        LUMEN_CHECK_EQ( ( *Commander )->GetLastError(), 0U );
    }

    LUMEN_TEST( FHierarchyDeferredCommands, StaleParentIdIsRejectedStructurally )
    {
        FWorld World( MakeHierarchyDesc( 1U, 16ULL ) );

        auto ChildResult  = World.TrySpawnActor<FHierarchyCommandActor>( &World, InvalidActorID, InvalidActorID, 99U );
        auto ParentResult = World.TrySpawnActor<FHierarchyCommandActor>( &World, InvalidActorID, InvalidActorID, 99U );
        LUMEN_CHECK_HAS_VALUE( ChildResult );
        LUMEN_CHECK_HAS_VALUE( ParentResult );

        const ActorID ChildId  = ( *ChildResult )->GetId();
        const ActorID ParentId = ( *ParentResult )->GetId();
        LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ChildId ) );
        LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ParentId ) );

        LUMEN_CHECK_HAS_VALUE( World.TryDestroyActor( ParentId ) );
        const TExpected<void, ESceneHierarchyError> Stale = World.TryHierarchyAttach( ChildId, ParentId );
        LUMEN_CHECK_HAS_ERROR( Stale );
        LUMEN_CHECK_TRUE( Stale.Error() == ESceneHierarchyError::StaleParent );
    }

    LUMEN_TEST( FHierarchyDeferredCommands, SaturatedQueueIsReportedWithoutLaterCommands )
    {
        FWorld World( MakeHierarchyDesc( 1U, 1ULL ) );

        auto ChildResult  = World.TrySpawnActor<FHierarchyCommandActor>( &World, InvalidActorID, InvalidActorID, 99U );
        auto ParentResult = World.TrySpawnActor<FHierarchyCommandActor>( &World, InvalidActorID, InvalidActorID, 99U );
        LUMEN_CHECK_HAS_VALUE( ChildResult );
        LUMEN_CHECK_HAS_VALUE( ParentResult );

        const ActorID ChildId  = ( *ChildResult )->GetId();
        const ActorID ParentId = ( *ParentResult )->GetId();
        LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ChildId ) );
        LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ParentId ) );

        auto First  = World.TrySpawnActor<FHierarchyCommandActor>( &World, ChildId, ParentId, 0U );
        auto Second = World.TrySpawnActor<FHierarchyCommandActor>( &World, ChildId, ParentId, 0U );
        LUMEN_CHECK_HAS_VALUE( First );
        LUMEN_CHECK_HAS_VALUE( Second );

        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
        LUMEN_CHECK_TRUE( ( *First )->GetLastError() == 0U or ( *Second )->GetLastError() == 0U );
        LUMEN_CHECK_TRUE( ( *First )->GetLastError() != 0U or ( *Second )->GetLastError() != 0U );
        LUMEN_CHECK_EQ( World.GetHierarchy().GetParentId( ChildId ).ValueOr( InvalidActorID ), InvalidActorID );
    }

    LUMEN_INTEGRATION_TEST( FHierarchyDeferredCommands, CanonicalDigestIsStableAcrossRunsAndWorkers )
    {
        constexpr UInt64 Seeds[]        = { 17ULL, 0x5EEDULL, 987654321ULL };
        constexpr UInt32 WorkerCounts[] = { 1U, 2U, 4U };
        constexpr UInt32 Runs           = 32U;
        constexpr UInt32 TickCount      = 8U;

        for ( const UInt64 Seed : Seeds )
        {
            UInt64 CanonicalDigest = 0ULL;
            Bool bHasCanonical     = false;

            for ( const UInt32 WorkerCount : WorkerCounts )
            {
                for ( UInt32 RunIndex = 0U; RunIndex < Runs; ++RunIndex )
                {
                    FWorld World( MakeHierarchyDesc( WorkerCount, 16ULL ) );
                    static_cast<void>( Seed );

                    auto ChildResult = World.TrySpawnActor<FHierarchyCommandActor>( &World, InvalidActorID, InvalidActorID, 99U );
                    auto ParentResult =
                        World.TrySpawnActor<FHierarchyCommandActor>( &World, InvalidActorID, InvalidActorID, 99U );
                    LUMEN_CHECK_HAS_VALUE( ChildResult );
                    LUMEN_CHECK_HAS_VALUE( ParentResult );

                    const ActorID ChildId  = ( *ChildResult )->GetId();
                    const ActorID ParentId = ( *ParentResult )->GetId();
                    LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ChildId ) );
                    LUMEN_CHECK_HAS_VALUE( World.TryRegisterHierarchyActor( ParentId ) );

                    /** INFO: Conflicting reparenting commands are resolved by the canonical deferred-command order. */
                    auto First  = World.TrySpawnActor<FHierarchyCommandActor>( &World, ChildId, ParentId, 0U );
                    auto Second = World.TrySpawnActor<FHierarchyCommandActor>( &World, ParentId, ChildId, 0U );
                    LUMEN_CHECK_HAS_VALUE( First );
                    LUMEN_CHECK_HAS_VALUE( Second );
                    static_cast<void>( RunIndex );

                    for ( UInt32 TickIndex = 0U; TickIndex < TickCount; ++TickIndex )
                    {
                        LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
                    }

                    const UInt64 Digest = HierarchyDigest( World );
                    if ( not bHasCanonical )
                    {
                        CanonicalDigest = Digest;
                        bHasCanonical   = true;
                    }
                    LUMEN_CHECK_EQ( Digest, CanonicalDigest );
                }
            }
        }
    }

} // namespace
} // namespace Lumen
