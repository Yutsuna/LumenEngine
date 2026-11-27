/**
 * @file RHIBackendCast.inl
 * @brief Implements checked backend casts.
 */

#pragma once

#include <RHI/Device/RHIBackendCast.hpp>

/**
 * Public
 */

template <typename BackendType> Lumen::TOptional<BackendType *> Lumen::RHI::TryRHIBackendCast ( IRHIObject &InObject ) noexcept
{
    if ( InObject.GetBackendTypeTag() != FHash::TypeHash<BackendType>() )
    {
        return {};
    }
    return TOptional<BackendType *>( static_cast<BackendType *>( &InObject ) );
}

template <typename BackendType>
Lumen::TOptional<Lumen::TSharedPtr<BackendType>> Lumen::RHI::TryRHIBackendCast ( const TSharedRef<IRHIObject> &InObject ) noexcept
{
    if ( InObject->GetBackendTypeTag() != FHash::TypeHash<BackendType>() )
    {
        return {};
    }
    const TSharedPtr<IRHIObject> Base = InObject;
    return TOptional<TSharedPtr<BackendType>>( StaticCastSharedPtr<BackendType>( Base ) );
}
