/**
 * @file RHIValidation.inl
 * @brief Implements typed validation unwrapping.
 */

#pragma once

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Validation/RHIValidation.hpp>

template <typename ObjectType> ObjectType *Lumen::RHI::UnwrapRHI ( ObjectType *InObject ) noexcept
{
    return InObject == nullptr ? nullptr : static_cast<ObjectType *>( UnwrapRHIObject( static_cast<IRHIObject *>( InObject ) ) );
}

template <typename ObjectType> const ObjectType *Lumen::RHI::UnwrapRHI ( const ObjectType *InObject ) noexcept
{
    return InObject == nullptr
               ? nullptr
               : static_cast<const ObjectType *>( UnwrapRHIObject( static_cast<const IRHIObject *>( InObject ) ) );
}

template <typename ObjectType> ObjectType *Lumen::RHI::UnwrapRHI ( const TSharedRef<ObjectType> &InObject ) noexcept
{
    return UnwrapRHI( InObject.Get() );
}

template <typename ObjectType>
Lumen::TSharedRef<ObjectType> Lumen::RHI::UnwrapRHIShared ( const TSharedRef<ObjectType> &InObject ) noexcept
{
    const TSharedRef<IRHIObject> Base = InObject;
    return StaticCastSharedRef<ObjectType>( UnwrapRHISharedObject( Base ) );
}

template <typename ObjectType>
Lumen::TSharedPtr<ObjectType> Lumen::RHI::UnwrapRHIShared ( const TSharedPtr<ObjectType> &InObject ) noexcept
{
    if ( not InObject )
    {
        return {};
    }
    const TSharedPtr<IRHIObject> Base = StaticCastSharedPtr<IRHIObject>( InObject );
    return StaticCastSharedPtr<ObjectType>( UnwrapRHISharedObject( Base ) );
}

template <typename BackendType> Lumen::TOptional<BackendType *> Lumen::RHI::TryUnwrapRHIBackend ( IRHIObject *InObject ) noexcept
{
    IRHIObject *Unwrapped = UnwrapRHIObject( InObject );
    return Unwrapped == nullptr ? Lumen::TOptional<BackendType *>{} : TryRHIBackendCast<BackendType>( *Unwrapped );
}
