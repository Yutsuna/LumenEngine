/**
 * @file CameraRegistryTests.cpp
 * @brief Functional contracts for the bounded camera component registry.
 */

#include <Camera/CameraRegistry.hpp>

#include <Rendering/RenderRecords.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( FCameraComponentRegistry, RegistrationRejectsDuplicatesAndOverflow )
{
    FCameraComponentRegistry Registry( 1ULL );
    LUMEN_CHECK_EQ( Registry.GetCapacity(), 1ULL );
    LUMEN_CHECK_EQ( Registry.GetCount(), 0ULL );

    LUMEN_CHECK_HAS_VALUE( Registry.TryRegister( 11ULL, FCameraComponentDesc{} ) );
    LUMEN_CHECK_EQ( Registry.GetCount(), 1ULL );
    LUMEN_CHECK_HAS_ERROR( Registry.TryRegister( 11ULL, FCameraComponentDesc{} ) );
    LUMEN_CHECK_HAS_ERROR( Registry.TryRegister( 12ULL, FCameraComponentDesc{} ) );

    LUMEN_CHECK_HAS_VALUE( Registry.TryUnregister( 11ULL ) );
    LUMEN_CHECK_EQ( Registry.GetCount(), 0ULL );
    LUMEN_CHECK_HAS_VALUE( Registry.TryRegister( 12ULL, FCameraComponentDesc{} ) );
    LUMEN_CHECK_EQ( Registry.GetCount(), 1ULL );
}

LUMEN_TEST( FCameraComponentRegistry, MutationsApplyToTheCollectedRecord )
{
    FCameraComponentRegistry Registry( 2ULL );
    LUMEN_CHECK_HAS_VALUE( Registry.TryRegister( 21ULL, FCameraComponentDesc{ .Flags = ERenderCameraFlags::Active } ) );

    const Maths::FMat4x4f View =
        Maths::FMat4x4f::LookAt( Maths::FVec3f{ 0.0F, 0.0F, 4.0F }, Maths::FVec3f{ 0.0F }, Maths::FVec3f{ 0.0F, 1.0F, 0.0F } );
    const Maths::FMat4x4f Projection = Maths::FMat4x4f::Perspective( 1.0F, 1.5F, 0.25F, 50.0F );
    LUMEN_CHECK_HAS_VALUE( Registry.TrySetView( 21ULL, View ) );
    LUMEN_CHECK_HAS_VALUE( Registry.TrySetProjection( 21ULL, Projection ) );
    LUMEN_CHECK_HAS_VALUE( Registry.TrySetClipping( 21ULL, 0.25F, 50.0F, FRenderViewport{} ) );
    LUMEN_CHECK_HAS_VALUE( Registry.TrySetFlags( 21ULL, ERenderCameraFlags::None ) );
    LUMEN_CHECK_HAS_ERROR( Registry.TrySetView( 99ULL, View ) );

    TVector<FRenderCameraRecord> Records;
    Records.reserve( 2ULL );
    Registry.CollectRecords( Records );
    LUMEN_CHECK_EQ( Records.size(), 1ULL );
    LUMEN_CHECK_EQ( Records[0ULL].ProxyId.GetValue(), 21ULL );
    LUMEN_CHECK_EQ( Records[0ULL].View, View );
    LUMEN_CHECK_EQ( Records[0ULL].Projection, Projection );
    LUMEN_CHECK_EQ( Records[0ULL].NearPlane, 0.25F );
    LUMEN_CHECK_EQ( Records[0ULL].FarPlane, 50.0F );
    LUMEN_CHECK_FALSE( HasAnyFlags( Records[0ULL].Flags, ERenderCameraFlags::Active ) );
}

} // namespace Lumen
