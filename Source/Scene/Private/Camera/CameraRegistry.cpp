/**
 * @file CameraRegistry.cpp
 * @brief Implements the bounded camera component registry.
 */

#include <Camera/CameraRegistry.hpp>

#include <Algo/ForEach.hpp>
#include <HAL/LockGuard.hpp>
#include <Logging/LoggingCategory.hpp>
#include <Templates/Unexpected.hpp>

/**
 * Public
 */

const Lumen::FLogCategory Lumen::LogCamera( "Camera" );

Lumen::FCameraComponentRegistry::FCameraComponentRegistry ( const USize InCapacity ) noexcept : Capacity( InCapacity )
{
    /** INFO: Every slot and index bucket is reserved once here; registration and mutation never allocate. */
    Entries.reserve( Capacity );
    IdToIndex.reserve( Capacity );
}

Lumen::TExpected<void, Lumen::ECameraComponentError>
Lumen::FCameraComponentRegistry::TryRegister ( const ActorID InId, const FCameraComponentDesc &InDesc ) noexcept
{
    const HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );

    if ( IdToIndex.contains( InId ) )
    {
        return MakeUnexpected( ECameraComponentError::AlreadyRegistered );
    }

    if ( Entries.size() >= Capacity ) [[unlikely]]
    {
        return MakeUnexpected( ECameraComponentError::CapacityExceeded );
    }

    const USize NewIndex = Entries.size();
    Entries.emplace_back( FCameraEntry{ .Id = InId, .Component = FCameraComponent{} } );
    FCameraComponent &Component = Entries.back().Component;
    Component.View              = InDesc.View;
    Component.Projection        = InDesc.Projection;
    Component.NearPlane         = InDesc.NearPlane;
    Component.FarPlane          = InDesc.FarPlane;
    Component.Viewport          = InDesc.Viewport;
    Component.Flags             = InDesc.Flags;
    IdToIndex.emplace( InId, NewIndex );
    return {};
}

Lumen::TExpected<void, Lumen::ECameraComponentError>
Lumen::FCameraComponentRegistry::TryUnregister ( const ActorID InId ) noexcept
{
    const HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );
    const TMap<ActorID, USize>::const_iterator It = IdToIndex.find( InId );

    if ( It == IdToIndex.end() )
    {
        return MakeUnexpected( ECameraComponentError::UnknownActor );
    }

    const USize IndexToRemove = It->second;
    const USize LastIndex     = Entries.size() - 1U;

    if ( IndexToRemove != LastIndex )
    {
        const ActorID LastId   = Entries[LastIndex].Id;
        Entries[IndexToRemove] = Entries[LastIndex];
        IdToIndex[LastId]      = IndexToRemove;
    }

    Entries.pop_back();
    IdToIndex.erase( InId );
    return {};
}

Lumen::TExpected<void, Lumen::ECameraComponentError>
Lumen::FCameraComponentRegistry::TrySetView ( const ActorID InId, const Maths::FMat4x4f &InView ) noexcept
{
    const HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );
    return FindComponent( InId )
        .Transform( [&InView] ( FCameraComponent *const InComponent ) noexcept -> TExpected<void, ECameraComponentError>
    {
        InComponent->View = InView;
        return {};
    } ).ValueOr( MakeUnexpected( ECameraComponentError::UnknownActor ) );
}

Lumen::TExpected<void, Lumen::ECameraComponentError>
Lumen::FCameraComponentRegistry::TrySetProjection ( const ActorID InId, const Maths::FMat4x4f &InProjection ) noexcept
{
    const HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );
    return FindComponent( InId )
        .Transform( [&InProjection] ( FCameraComponent *const InComponent ) noexcept -> TExpected<void, ECameraComponentError>
    {
        InComponent->Projection = InProjection;
        return {};
    } ).ValueOr( MakeUnexpected( ECameraComponentError::UnknownActor ) );
}

Lumen::TExpected<void, Lumen::ECameraComponentError> Lumen::FCameraComponentRegistry::TrySetClipping (
    const ActorID InId, const Float32 InNearPlane, const Float32 InFarPlane, const FRenderViewport &InViewport ) noexcept
{
    const HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );
    return FindComponent( InId )
        .Transform( [&InNearPlane, &InFarPlane,
                     &InViewport] ( FCameraComponent *const InComponent ) noexcept -> TExpected<void, ECameraComponentError>
    {
        InComponent->NearPlane = InNearPlane;
        InComponent->FarPlane  = InFarPlane;
        InComponent->Viewport  = InViewport;
        return {};
    } ).ValueOr( MakeUnexpected( ECameraComponentError::UnknownActor ) );
}

Lumen::TExpected<void, Lumen::ECameraComponentError>
Lumen::FCameraComponentRegistry::TrySetFlags ( const ActorID InId, const ERenderCameraFlags InFlags ) noexcept
{
    const HAL::TSharedUniqueLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );
    return FindComponent( InId )
        .Transform( [InFlags] ( FCameraComponent *const InComponent ) noexcept -> TExpected<void, ECameraComponentError>
    {
        InComponent->Flags = InFlags;
        return {};
    } ).ValueOr( MakeUnexpected( ECameraComponentError::UnknownActor ) );
}

void Lumen::FCameraComponentRegistry::CollectRecords ( TVector<FRenderCameraRecord> &OutRecords ) const noexcept
{
    const HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );

    Algo::ForEach( Entries, [&OutRecords] ( const FCameraEntry &Entry ) noexcept
    {
        OutRecords.emplace_back( FRenderCameraRecord{
            .ProxyId    = FRenderProxyId{ Entry.Id },
            .View       = Entry.Component.View,
            .Projection = Entry.Component.Projection,
            .NearPlane  = Entry.Component.NearPlane,
            .FarPlane   = Entry.Component.FarPlane,
            .Viewport   = Entry.Component.Viewport,
            .Flags      = Entry.Component.Flags,
        } );
    } );
}

Lumen::USize Lumen::FCameraComponentRegistry::GetCapacity () const noexcept
{
    return Capacity;
}

Lumen::USize Lumen::FCameraComponentRegistry::GetCount () const noexcept
{
    const HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( RegistryMutex );
    return Entries.size();
}

Lumen::TOptional<Lumen::FCameraComponent *> Lumen::FCameraComponentRegistry::FindComponent ( const ActorID InId ) noexcept
{
    const TMap<ActorID, USize>::const_iterator It = IdToIndex.find( InId );
    if ( It == IdToIndex.end() )
    {
        return {};
    }
    return TOptional<FCameraComponent *>( &Entries[It->second].Component );
}
