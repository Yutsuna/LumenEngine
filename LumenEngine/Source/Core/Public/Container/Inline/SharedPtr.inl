/**
 * @file SharedPtr.inl
 * @brief Inline implementations for Shared Pointer classes.
 */

#pragma once

#include "Container/SharedPtr.hpp"

#include "Container/Inline/SharedPtrInternal.inl"

namespace LumenEngine
{

template <typename Type> TSharedPtr<Type>::TSharedPtr() : Object( nullptr ), Controller( nullptr )
{
    /* Empty */
}

template <typename Type> TSharedPtr<Type>::TSharedPtr( NullptrType ) : Object( nullptr ), Controller( nullptr )
{
    /* Empty */
}

template <typename Type>
template <typename OtherType>
    requires Concepts::CConvertibleTo<OtherType *, Type *>
TSharedPtr<Type>::TSharedPtr( const SharedPtrInternal::TRawPtrProxy<OtherType> &Proxy )
{
    Object = Proxy.Object;

    if ( Proxy.Object )
    {
        Controller = SharedPtrInternal::NewDefaultReferenceController<OtherType>( Proxy.Object );
    }
    else
    {
        Controller = nullptr;
    }
}

template <typename Type>
template <typename OtherType>
    requires Concepts::CConvertibleTo<OtherType *, Type *>
TSharedPtr<Type>::TSharedPtr( const TSharedRef<OtherType> &Other ) : Object( Other.Object ), Controller( Other.Controller )
{
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
}

template <typename Type> TSharedPtr<Type>::TSharedPtr( const TSharedPtr &Other ) : Object( Other.Object ), Controller( Other.Controller )
{
    if ( Controller )
    {
        Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
    }
}

template <typename Type> TSharedPtr<Type>::TSharedPtr( TSharedPtr &&Other ) : Object( Other.Object ), Controller( Other.Controller )
{
    Other.Object     = nullptr;
    Other.Controller = nullptr;
}

template <typename Type> TSharedPtr<Type>::~TSharedPtr()
{
    Release();
}

template <typename Type> TSharedPtr<Type> &TSharedPtr<Type>::operator=( const TSharedPtr &Other )
{
    if ( this != &Other )
    {
        Release();
        Object     = Other.Object;
        Controller = Other.Controller;
        if ( Controller )
        {
            Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
        }
    }
    return *this;
}

template <typename Type> TSharedPtr<Type> &TSharedPtr<Type>::operator=( const TSharedRef<Type> &Other )
{
    Release();
    Object     = Other.Object;
    Controller = Other.Controller;
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
    return *this;
}

template <typename Type> Type &TSharedPtr<Type>::operator*() const
{
    assert( IsValid() );
    return *Object;
}

template <typename Type> Type *TSharedPtr<Type>::operator->() const
{
    assert( IsValid() );
    return Object;
}

template <typename Type> Type *TSharedPtr<Type>::Get () const
{
    return Object;
}

template <typename Type> bool TSharedPtr<Type>::IsValid () const
{
    return Object != nullptr;
}

template <typename Type> TSharedPtr<Type>::operator bool () const
{
    return IsValid();
}

template <typename Type> void TSharedPtr<Type>::Reset ()
{
    Release();
    Object     = nullptr;
    Controller = nullptr;
}

template <typename Type> void TSharedPtr<Type>::Release ()
{
    if ( Controller && Controller->SharedCount.fetch_sub( 1, std::memory_order_acq_rel ) == 1 )
    {
        Controller->DestroyObject();
        Controller->Deallocate();
    }
}

/**
 * SharedPtr Builders
 */

template <typename ObjectType, typename... Arguments> static inline TSharedRef<ObjectType> MakeShared ( Arguments &&...InArgs )
{
    SharedPtrInternal::TIntrusiveReferenceController<ObjectType> *Controller =
        SharedPtrInternal::NewIntrusiveReferenceController<ObjectType>( std::forward<Arguments>( InArgs )... );
    return MakeSharedRef<ObjectType>( Controller->GetObjectPtr(), static_cast<SharedPtrInternal::FReferenceController *>( Controller ) );
}

template <typename ObjectType> static inline SharedPtrInternal::TRawPtrProxy<ObjectType> MakeShareable ( ObjectType *InObject )
{
    return SharedPtrInternal::TRawPtrProxy<ObjectType>( InObject );
}

/**
 * StaticCast
 */

template <typename CastToType, typename CastFromType> static inline TSharedPtr<CastToType> StaticCastSharedPtr ( const TSharedPtr<CastFromType> &InSharedPtr )
{
    if ( InSharedPtr.IsValid() )
    {
        InSharedPtr.Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
        return MakeSharedRef<CastToType>( static_cast<CastToType *>( InSharedPtr.Object ), InSharedPtr.Controller );
    }

    return TSharedPtr<CastToType>();
}

} // namespace LumenEngine
