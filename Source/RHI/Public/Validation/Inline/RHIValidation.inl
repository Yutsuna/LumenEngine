/**
 * @file RHIValidation.inl
 * @brief Implements typed validation unwrapping.
 */
#pragma once

#include <Device/RHIBackendCast.hpp>
#include <Validation/RHIValidation.hpp>

template <typename ObjectType> ObjectType *Lumen::UnwrapRHI ( ObjectType *InObject ) noexcept
{
    return InObject == nullptr ? nullptr : static_cast<ObjectType *>( UnwrapRHIObject( static_cast<IRHIObject *>( InObject ) ) );
}

template <typename ObjectType> const ObjectType *Lumen::UnwrapRHI ( const ObjectType *InObject ) noexcept
{
    return InObject == nullptr
               ? nullptr
               : static_cast<const ObjectType *>( UnwrapRHIObject( static_cast<const IRHIObject *>( InObject ) ) );
}

template <typename ObjectType> ObjectType *Lumen::UnwrapRHI ( const TSharedRef<ObjectType> &InObject ) noexcept
{
    return UnwrapRHI( InObject.Get() );
}

template <typename ObjectType>
Lumen::TSharedRef<ObjectType> Lumen::UnwrapRHIShared ( const TSharedRef<ObjectType> &InObject ) noexcept
{
    const TSharedRef<IRHIObject> Base = InObject;
    return StaticCastSharedRef<ObjectType>( UnwrapRHISharedObject( Base ) );
}

template <typename ObjectType>
Lumen::TSharedPtr<ObjectType> Lumen::UnwrapRHIShared ( const TSharedPtr<ObjectType> &InObject ) noexcept
{
    if ( not InObject )
    {
        return {};
    }
    const TSharedPtr<IRHIObject> Base = StaticCastSharedPtr<IRHIObject>( InObject );
    return StaticCastSharedPtr<ObjectType>( UnwrapRHISharedObject( Base ) );
}

template <typename BackendType> Lumen::TOptional<BackendType *> Lumen::TryUnwrapRHIBackend ( IRHIObject *InObject ) noexcept
{
    IRHIObject *Unwrapped = UnwrapRHIObject( InObject );
    return Unwrapped == nullptr ? Lumen::TOptional<BackendType *>{} : TryRHIBackendCast<BackendType>( *Unwrapped );
}
