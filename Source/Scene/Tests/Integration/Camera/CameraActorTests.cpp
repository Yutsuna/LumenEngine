/**
 * @file CameraActorTests.cpp
 * @brief Integration contracts for the generic camera actor and snapshot publication.
 */

#include <Camera/CameraActor.hpp>

#include <Messages/EngineMessageTypes.hpp>
#include <Rendering/RenderSnapshotView.hpp>
#include <Tests/UnitTest.hpp>
#include <World/World.hpp>

namespace Lumen
{

namespace
{

    [[nodiscard]] FWorldDesc MakeDesc () noexcept
    {
        FWorldDesc Desc;
        Desc.WorkerCount      = 1U;
        Desc.MaxActors        = 4ULL;
        Desc.RenderCapacities = FRenderSnapshotCapacities{
            .TransformCapacity = 4ULL,
            .CameraCapacity    = 2ULL,
        };
        return Desc;
    }

} // namespace

LUMEN_INTEGRATION_TEST( ACameraActor, PublishesLookAtPoseAndCameraRecord )
{
    FWorld World( MakeDesc() );
    const FCameraComponentDesc CameraDesc{ .Flags = ERenderCameraFlags::Active };
    const auto Spawned = World.TrySpawnActor<ACameraActor>( World.GetSpatialRegistry(), World.GetCameraRegistry() );
    LUMEN_CHECK_HAS_VALUE( Spawned );
    const ActorID CameraId = ( *Spawned )->GetId();
    LUMEN_CHECK_HAS_VALUE( World.GetSpatialRegistry().TryRegisterSpatialEntity( CameraId ) );
    LUMEN_CHECK_HAS_VALUE( World.TryAddCameraComponent( CameraId, CameraDesc ) );
    LUMEN_CHECK_EQ( World.GetCameraRegistry().GetCount(), 1ULL );

    const FCameraLookAtPayload LookAt{ .Eye    = Maths::FVec3f{ 1.0F, 2.0F, 5.0F },
                                       .Target = Maths::FVec3f{ 0.0F, 0.0F, 0.0F },
                                       .Up     = Maths::FVec3f{ 0.0F, 1.0F, 0.0F } };
    LUMEN_CHECK_TRUE( ( *Spawned )
                          ->EnqueueMessage( FMessage::Make( static_cast<UInt32>( EEngineMessage::CameraSetLookAt ),
                                                            InvalidActorID, LookAt ) ) );
    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
    LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );

    const FRenderSnapshotView View = World.GetRenderSnapshot();
    LUMEN_CHECK_EQ( View.Cameras.size(), 1ULL );
    LUMEN_CHECK_EQ( View.Cameras[0ULL].ProxyId.GetValue(), CameraId );
    LUMEN_CHECK_TRUE( HasAnyFlags( View.Cameras[0ULL].Flags, ERenderCameraFlags::Active ) );

    const Maths::FVec3f Eye = Maths::FVec3f( View.Cameras[0ULL].View[3].Data[0], View.Cameras[0ULL].View[3].Data[1],
                                             View.Cameras[0ULL].View[3].Data[2] );
    /** INFO: View translation is -R dot Eye, not Eye; the WorldTransform column 3 holds Eye. */
    LUMEN_CHECK( Eye != LookAt.Eye );

    const FSpatialCommittedView Committed = World.GetSpatialRegistry().GetCommittedView();
    LUMEN_CHECK_EQ( Committed.EntityIDs.size(), 1ULL );
    const Maths::FMat4x4f &WorldTransform = Committed.Transforms[0ULL];
    LUMEN_CHECK_EQ( WorldTransform[3].Data[0], LookAt.Eye.X );
    LUMEN_CHECK_EQ( WorldTransform[3].Data[1], LookAt.Eye.Y );
    LUMEN_CHECK_EQ( WorldTransform[3].Data[2], LookAt.Eye.Z );
}

LUMEN_INTEGRATION_TEST( ACameraActor, SetParamsAndDestroyUpdateThenRemoveTheComponent )
{
    FWorld World( MakeDesc() );
    const auto Spawned = World.TrySpawnActor<ACameraActor>( World.GetSpatialRegistry(), World.GetCameraRegistry() );
    LUMEN_CHECK_HAS_VALUE( Spawned );
    const ActorID CameraId = ( *Spawned )->GetId();
    LUMEN_CHECK_HAS_VALUE( World.GetSpatialRegistry().TryRegisterSpatialEntity( CameraId ) );
    LUMEN_CHECK_HAS_VALUE( World.TryAddCameraComponent( CameraId, FCameraComponentDesc{} ) );

    const FCameraParamsPayload Params{
        .FieldOfViewDegrees = 46.0F, .AspectRatio = 16.0F / 9.0F, .NearPlane = 0.5F, .FarPlane = 100.0F };
    LUMEN_CHECK_TRUE( ( *Spawned )
                          ->EnqueueMessage( FMessage::Make( static_cast<UInt32>( EEngineMessage::CameraSetParams ),
                                                            InvalidActorID, Params ) ) );
    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );

    LUMEN_CHECK_HAS_VALUE( World.TryDestroyActor( CameraId ) );
    LUMEN_CHECK_EQ( World.GetCameraRegistry().GetCount(), 0ULL );
    LUMEN_CHECK_HAS_VALUE( World.TryTick( 0.016 ) );
    LUMEN_CHECK_TRUE( World.SwapRenderSnapshotReadBuffers() );
    LUMEN_CHECK_EQ( World.GetRenderSnapshot().Cameras.size(), 0ULL );
}

} // namespace Lumen
