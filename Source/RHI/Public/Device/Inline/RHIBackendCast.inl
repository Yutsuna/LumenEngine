/**
 * @file RHIBackendCast.inl
 * @brief Implements checked backend casts.
 */
#pragma once

#include <Device/RHIBackendCast.hpp>

template <typename BackendType> BackendType *Lumen::TryRHIBackendCast ( IRHIObject &InObject ) noexcept
{
    return InObject.GetBackendTypeTag() == FHash::TypeHash<BackendType>() ? static_cast<BackendType *>( &InObject ) : nullptr;
}

template <typename BackendType>
Lumen::TOptional<Lumen::TSharedPtr<BackendType>> Lumen::TryRHIBackendCast ( const TSharedRef<IRHIObject> &InObject ) noexcept
{
    if ( InObject->GetBackendTypeTag() != FHash::TypeHash<BackendType>() )
    {
        return {};
    }
    const TSharedPtr<IRHIObject> Base = InObject;
    return TOptional<TSharedPtr<BackendType>>( StaticCastSharedPtr<BackendType>( Base ) );
}
