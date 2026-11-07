/*
 * @file WorldSpatialRegistryTests.cpp
 * @brief Functional tests for FSpatialRegistry.
 */

#include <World/WorldSpatialRegistry.hpp>

#include <Maths/Matrix.hpp>
#include <Maths/Vec.hpp>
#include <Tests/UnitTest.hpp>

#include <thread>

namespace Lumen
{

/**
 * Test helpers
 */

namespace
{

    /**
    * @brief Wraps the shared registry with a defensive cleanup helper: each test
            wipes every id it registered, then flushes one final snapshot, leaving
            the singleton pristine for the following cases. The destructor keeps
            this discipline even when an assertion aborts the test body halfway.
     */
    struct FRegistrySession
    {
        FSpatialRegistry &Registry = FSpatialRegistry::Get();
        TVector<ActorID> TrackedIds;

        ~FRegistrySession ()
        {
            Cleanup();
        }

        void TrackRegister ( const ActorID InId )
        {
            Registry.RegisterSpatialEntity( InId );
            TrackedIds.push_back( InId );
        }

        void Cleanup () const
        {
            for ( const ActorID EntityId : TrackedIds )
            {
                Registry.UnregisterSpatialEntity( EntityId );
            }

            Registry.Publish();
            Registry.SwapReadBuffers();
        }
    };

    [[nodiscard]] Maths::FMatrix4x4f MakeTranslation ( const Float32 InX, const Float32 InY, const Float32 InZ ) noexcept
    {
        return Maths::FMatrix4x4f::Translate( Maths::FVec3f{ InX, InY, InZ } );
    }

} // namespace

/**
 * Singleton
 */

LUMEN_TEST( FSpatialRegistry_Singleton, AlwaysReturnsSameInstance )
{
    FSpatialRegistry &First  = FSpatialRegistry::Get();
    FSpatialRegistry &Second = FSpatialRegistry::Get();

    LUMEN_CHECK_TRUE( &First == &Second );
}

/**
 * Register
 */

LUMEN_TEST( FSpatialRegistry_Register, PublishesEntityWithInitialTransform )
{
    FRegistrySession Session;
    Session.Cleanup();

    constexpr ActorID EntityId = 7001ULL;

    /** INFO: Registration is invisible until a publication is swapped in. */
    Session.Registry.RegisterSpatialEntity( EntityId );
    Session.TrackedIds.push_back( EntityId );

    LUMEN_CHECK_TRUE( Session.Registry.GetReadSnapshot().EntityIDs.empty() );

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const FSpatialRegistryData &Snapshot = Session.Registry.GetReadSnapshot();

    LUMEN_CHECK_EQ( Snapshot.EntityIDs.size(), 1ULL );
    LUMEN_CHECK_EQ( Snapshot.EntityIDs[0], EntityId );
    LUMEN_CHECK_TRUE( Snapshot.Transforms[0] == Maths::FMatrix4x4f::Identity() );

    Session.Cleanup();
}

LUMEN_TEST( FSpatialRegistry_Register, IgnoresDuplicateRegistrations )
{
    FRegistrySession Session;
    Session.Cleanup();

    constexpr ActorID EntityId = 7002ULL;

    /** INFO: The first registration is tracked; the duplicate goes through the plain API. */
    Session.TrackRegister( EntityId );
    Session.Registry.RegisterSpatialEntity( EntityId );
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const FSpatialRegistryData &Snapshot = Session.Registry.GetReadSnapshot();

    LUMEN_CHECK_EQ( Snapshot.EntityIDs.size(), 1ULL );
    LUMEN_CHECK_EQ( Snapshot.EntityIDs[0], EntityId );

    Session.Cleanup();
}

LUMEN_TEST( FSpatialRegistry_Register, KeepsRegistrationOrder )
{
    FRegistrySession Session;
    Session.Cleanup();

    constexpr ActorID FirstId  = 7003ULL;
    constexpr ActorID SecondId = 7004ULL;
    constexpr ActorID ThirdId  = 7005ULL;

    Session.TrackRegister( FirstId );
    Session.TrackRegister( SecondId );
    Session.TrackRegister( ThirdId );

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const FSpatialRegistryData &Snapshot = Session.Registry.GetReadSnapshot();

    LUMEN_CHECK_EQ( Snapshot.EntityIDs.size(), 3ULL );
    LUMEN_CHECK_EQ( Snapshot.EntityIDs[0], FirstId );
    LUMEN_CHECK_EQ( Snapshot.EntityIDs[1], SecondId );
    LUMEN_CHECK_EQ( Snapshot.EntityIDs[2], ThirdId );

    Session.Cleanup();
}

/**
 * UpdateTransform
 */

LUMEN_TEST( FSpatialRegistry_UpdateTransform, RoundTripsPublishedValue )
{
    FRegistrySession Session;
    Session.Cleanup();

    constexpr ActorID EntityId = 7006ULL;

    Session.TrackRegister( EntityId );

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const Maths::FMatrix4x4f TargetTransform = MakeTranslation( 1.0F, 2.0F, 3.0F );

    Session.Registry.UpdateTransform( EntityId, TargetTransform );
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const FSpatialRegistryData &Snapshot = Session.Registry.GetReadSnapshot();

    LUMEN_CHECK_EQ( Snapshot.EntityIDs.size(), 1ULL );
    LUMEN_CHECK_TRUE( Snapshot.Transforms[0] == TargetTransform );

    Session.Cleanup();
}

LUMEN_TEST( FSpatialRegistry_UpdateTransform, UnknownEntityIsHarmless )
{
    FRegistrySession Session;
    Session.Cleanup();

    constexpr ActorID EntityId  = 7007ULL;
    constexpr ActorID MissingId = 7999ULL;

    Session.TrackRegister( EntityId );
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    Session.Registry.UpdateTransform( MissingId, MakeTranslation( 9.0F, 9.0F, 9.0F ) );
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const FSpatialRegistryData &Snapshot = Session.Registry.GetReadSnapshot();

    LUMEN_CHECK_EQ( Snapshot.EntityIDs.size(), 1ULL );
    LUMEN_CHECK_TRUE( Snapshot.Transforms[0] == Maths::FMatrix4x4f::Identity() );

    Session.Cleanup();
}

/**
 * Unregister
 */

LUMEN_TEST( FSpatialRegistry_Unregister, ShiftsTailIntoVacantIndex )
{
    /**
     * INFO: Removal performs a swap of the last entity into the removed slot;
            the data must travel with the id it belongs to.
     */
    FRegistrySession Session;
    Session.Cleanup();

    constexpr ActorID HeadId = 7008ULL;
    constexpr ActorID MidId  = 7009ULL;
    constexpr ActorID TailId = 7010ULL;

    Session.TrackRegister( HeadId );
    Session.TrackRegister( MidId );
    Session.TrackRegister( TailId );

    const Maths::FMatrix4x4f MidTransform  = MakeTranslation( 1.0F, 0.0F, 0.0F );
    const Maths::FMatrix4x4f TailTransform = MakeTranslation( 0.0F, 2.0F, 0.0F );

    Session.Registry.UpdateTransform( MidId, MidTransform );
    Session.Registry.UpdateTransform( TailId, TailTransform );
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    Session.Registry.UnregisterSpatialEntity( HeadId );

    /** INFO: The removal stays invisible until it is published. */
    LUMEN_CHECK_EQ( Session.Registry.GetReadSnapshot().EntityIDs.size(), 3ULL );

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const FSpatialRegistryData &Snapshot = Session.Registry.GetReadSnapshot();

    LUMEN_CHECK_EQ( Snapshot.EntityIDs.size(), 2ULL );
    LUMEN_CHECK_EQ( Snapshot.EntityIDs[0], TailId );
    LUMEN_CHECK_EQ( Snapshot.EntityIDs[1], MidId );
    LUMEN_CHECK_TRUE( Snapshot.Transforms[0] == TailTransform );
    LUMEN_CHECK_TRUE( Snapshot.Transforms[1] == MidTransform );

    /** INFO: Removing the tail entity keeps the remaining order untouched. */
    Session.Registry.UnregisterSpatialEntity( TailId );
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const FSpatialRegistryData &TailSnapshot = Session.Registry.GetReadSnapshot();

    LUMEN_CHECK_EQ( TailSnapshot.EntityIDs.size(), 1ULL );
    LUMEN_CHECK_EQ( TailSnapshot.EntityIDs[0], MidId );
    LUMEN_CHECK_TRUE( TailSnapshot.Transforms[0] == MidTransform );

    Session.Cleanup();
}

LUMEN_TEST( FSpatialRegistry_Unregister, KeepsSnapshotStaleUntilPublished )
{
    FRegistrySession Session;
    Session.Cleanup();

    constexpr ActorID EntityId = 7011ULL;

    Session.TrackRegister( EntityId );
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    LUMEN_CHECK_EQ( Session.Registry.GetReadSnapshot().EntityIDs.size(), 1ULL );

    Session.Registry.UnregisterSpatialEntity( EntityId );

    LUMEN_CHECK_EQ( Session.Registry.GetReadSnapshot().EntityIDs.size(), 1ULL );

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    LUMEN_CHECK_TRUE( Session.Registry.GetReadSnapshot().EntityIDs.empty() );

    Session.Cleanup();
}

/**
 * Snapshot
 */

LUMEN_TEST( FSpatialRegistry_Snapshot, PublishWithoutSwapKeepsOldSnapshot )
{
    FRegistrySession Session;
    Session.Cleanup();

    constexpr ActorID EntityId = 7012ULL;

    Session.TrackRegister( EntityId );
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const Maths::FMatrix4x4f TargetTransform = MakeTranslation( 4.0F, 5.0F, 6.0F );

    Session.Registry.UpdateTransform( EntityId, TargetTransform );
    Session.Registry.Publish();

    /** INFO: No reader swap happened, so the old identity must still be visible. */
    LUMEN_CHECK_TRUE( Session.Registry.GetReadSnapshot().Transforms[0] == Maths::FMatrix4x4f::Identity() );

    Session.Registry.SwapReadBuffers();

    LUMEN_CHECK_TRUE( Session.Registry.GetReadSnapshot().Transforms[0] == TargetTransform );

    Session.Cleanup();
}

LUMEN_TEST( FSpatialRegistry_Snapshot, LaterPublishesSurpassEarlierOnes )
{
    /**
     * INFO: The reader always gets the freshest publication; an intermediate
            publication without swap is superseded rather than queued.
     */
    FRegistrySession Session;
    Session.Cleanup();

    constexpr ActorID FirstId  = 7013ULL;
    constexpr ActorID SecondId = 7014ULL;

    Session.TrackRegister( FirstId );

    Session.Registry.Publish();

    Session.TrackRegister( SecondId );

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const FSpatialRegistryData &Snapshot = Session.Registry.GetReadSnapshot();

    LUMEN_CHECK_EQ( Snapshot.EntityIDs.size(), 2ULL );
    LUMEN_CHECK_EQ( Snapshot.EntityIDs[0], FirstId );
    LUMEN_CHECK_EQ( Snapshot.EntityIDs[1], SecondId );

    Session.Cleanup();
}

/**
 * Threading
 */

LUMEN_TEST( FSpatialRegistry_Threading, UpdatesAndPublishesConcurrently )
{
    FRegistrySession Session;
    Session.Cleanup();

    constexpr ActorID EntityIds[3] = { 7015ULL, 7016ULL, 7017ULL };
    constexpr Float32 FinalOffset  = 10.0F;

    for ( const ActorID EntityId : EntityIds )
    {
        Session.TrackRegister( EntityId );
    }

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    std::thread Producers[3];

    for ( USize Index = 0ULL; Index < 3ULL; ++Index )
    {
        Producers[Index] = std::thread( [] ( const ActorID InEntityId ) noexcept -> void
        {
            constexpr Float32 ThreadFinalOffset = 10.0F;

            for ( Float32 Step = 1.0F; Step <= ThreadFinalOffset; Step += 1.0F )
            {
                FSpatialRegistry::Get().UpdateTransform( InEntityId, MakeTranslation( Step, Step, Step ) );
            }
        }, EntityIds[Index] );
    }

    std::thread Consumer( [&] () noexcept -> void
    {
        for ( UInt32 Cycle = 0U; Cycle < 64U; ++Cycle )
        {
            FSpatialRegistry::Get().Publish();
            FSpatialRegistry::Get().SwapReadBuffers();
        }
    } );

    for ( std::thread &Producer : Producers )
    {
        Producer.join();
    }

    Consumer.join();

    /** INFO: One final publication after joining captures the last writes. */
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const FSpatialRegistryData &Snapshot = Session.Registry.GetReadSnapshot();

    LUMEN_CHECK_EQ( Snapshot.EntityIDs.size(), 3ULL );

    ActorID ObservedSum = 0ULL;

    for ( const ActorID EntityId : Snapshot.EntityIDs )
    {
        ObservedSum += EntityId;
    }

    constexpr ActorID ExpectedSum = EntityIds[0] + EntityIds[1] + EntityIds[2];

    LUMEN_CHECK_EQ( ObservedSum, ExpectedSum );

    const Maths::FMatrix4x4f ExpectedTransform = MakeTranslation( FinalOffset, FinalOffset, FinalOffset );

    for ( ActorID EntityId : EntityIds )
    {
        const USize EntityIndex = [&] () -> USize
        {
            for ( USize SnapshotIndex = 0ULL; SnapshotIndex < Snapshot.EntityIDs.size(); ++SnapshotIndex )
            {
                if ( Snapshot.EntityIDs[SnapshotIndex] == EntityId )
                {
                    return SnapshotIndex;
                }
            }

            return 0ULL;
        }();

        LUMEN_CHECK_TRUE( Snapshot.Transforms[EntityIndex] == ExpectedTransform );
    }

    Session.Cleanup();
}

} // namespace Lumen
