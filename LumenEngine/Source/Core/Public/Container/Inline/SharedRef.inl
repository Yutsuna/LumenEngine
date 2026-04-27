/**
 * @file SharedRef.inl
 * @brief Inline implementations for Shared Reference classes.
 */

#pragma once

#include "Container/SharedPtr.hpp"

namespace LumenEngine
{

template <typename Type> TSharedRef<Type>::TSharedRef ( const TSharedRef &Other ) : Object( Other.Object ), Controller( Other.Controller )
{
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
}

template <typename Type> TSharedRef<Type>::TSharedRef ( TSharedRef &&Other ) : Object( Other.Object ), Controller( Other.Controller )
{
    Other.Controller = nullptr;
}

template <typename Type>
template <typename OtherType>
    requires Concepts::CConvertibleTo<OtherType *, Type *>
TSharedRef<Type>::TSharedRef( const SharedPtrInternal::TRawPtrProxy<OtherType> &Proxy )
    : Object( Proxy.Object ), Controller( new SharedPtrInternal::TDefaultReferenceController<OtherType>( Proxy.Object ) )
{
    assert( Object != nullptr );
}

template <typename Type>
template <typename OtherType>
    requires Concepts::CConvertibleTo<OtherType *, Type *>
TSharedRef<Type>::TSharedRef( const TSharedRef<OtherType> &Other ) : Object( Other.Object ), Controller( Other.Controller )
{
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
}

template <typename Type> TSharedRef<Type>::~TSharedRef ()
{
    Release();
}

template <typename Type> TSharedRef<Type> &TSharedRef<Type>::operator=( const TSharedRef &Other )
{
    if ( this != &Other )
    {
        Release();
        Object     = Other.Object;
        Controller = Other.Controller;
        Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
    }
    return *this;
}

template <typename Type> TSharedRef<Type> &TSharedRef<Type>::operator=( TSharedRef &&Other )
{
    if ( this != &Other )
    {
        Release();
        Object           = Other.Object;
        Controller       = Other.Controller;
        Other.Controller = nullptr;
    }
    return *this;
}

template <typename Type> Type &TSharedRef<Type>::operator*() const
{
    return *Object;
}

template <typename Type> Type *TSharedRef<Type>::operator->() const
{
    return Object;
}

template <typename Type> Type *TSharedRef<Type>::Get () const
{
    return Object;
}

template <typename Type> Int32 TSharedRef<Type>::GetSharedReferenceCount () const
{
    return Controller->SharedCount.load( std::memory_order_relaxed );
}

template <typename Type>
TSharedRef<Type>::TSharedRef ( Type *InObject, SharedPtrInternal::FReferenceController *InController ) : Object( InObject ), Controller( InController )
{
    assert( InObject != nullptr );
}

template <typename Type> void TSharedRef<Type>::Release ()
{
    if ( Controller and Controller->SharedCount.fetch_sub( 1, std::memory_order_acq_rel ) == 1 )
    {
        Controller->DestroyObject();
        Controller->Deallocate();
    }
}

/**
 * SharedRef Builder
 */

template <typename ObjectType> static inline TSharedRef<ObjectType> MakeSharedRef ( ObjectType *InObject, SharedPtrInternal::FReferenceController *InController )
{
    return TSharedRef<ObjectType>( InObject, InController );
}

/**
 * StaticCast
 */

template <typename CastToType, typename CastFromType> static inline TSharedRef<CastToType> StaticCastSharedRef ( TSharedRef<CastFromType> const &InSharedRef )
{
    InSharedRef.Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
    return MakeSharedRef<CastToType>( static_cast<CastToType *>( InSharedRef.Object ), InSharedRef.Controller );
}

} // namespace LumenEngine
