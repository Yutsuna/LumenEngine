///
///@file SharedPtr.inl
///@brief Inline implementations for Shared Pointer classes.
///

#pragma once

#include <Memory/SharedPtr.hpp>

#include <cassert>

namespace Lumen
{
template <typename Type> TSharedPtr<Type>::TSharedPtr() noexcept : Object( nullptr ), Controller( nullptr )
{
    /// Empty
}

template <typename Type> TSharedPtr<Type>::TSharedPtr( NullPtrType ) noexcept : Object( nullptr ), Controller( nullptr )
{
    /// Empty
}

template <typename Type>
template <typename OtherType>
    requires Concepts::CConvertibleTo<OtherType *, Type *>
TSharedPtr<Type>::TSharedPtr( const SharedPtrInternal::TRawPtrProxy<OtherType> &Proxy ) noexcept : Object( Proxy.Object )
{
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
TSharedPtr<Type>::TSharedPtr( const TSharedRef<OtherType> &Other ) noexcept
    : Object( Other.Object ), Controller( Other.Controller )
{
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
}

template <typename Type>
TSharedPtr<Type>::TSharedPtr( const TSharedPtr &Other ) noexcept : Object( Other.Object ), Controller( Other.Controller )
{
    if ( Controller != nullptr )
    {
        Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
    }
}

template <typename Type>
TSharedPtr<Type>::TSharedPtr( TSharedPtr &&Other ) noexcept : Object( Other.Object ), Controller( Other.Controller )
{
    Other.Object     = nullptr;
    Other.Controller = nullptr;
}

template <typename Type> TSharedPtr<Type>::~TSharedPtr() noexcept
{
    Release();
}

template <typename Type> TSharedPtr<Type> &TSharedPtr<Type>::operator=( const TSharedPtr &Other ) noexcept
{
    if ( this != &Other )
    {
        Release();
        Object     = Other.Object;
        Controller = Other.Controller;
        if ( Controller != nullptr )
        {
            Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
        }
    }
    return *this;
}

template <typename Type> TSharedPtr<Type> &TSharedPtr<Type>::operator=( const TSharedRef<Type> &Other ) noexcept
{
    Release();
    Object     = Other.Object;
    Controller = Other.Controller;
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
    return *this;
}

template <typename Type> Type &TSharedPtr<Type>::operator*() const noexcept
{
    assert( IsValid() );
    return *Object;
}

template <typename Type> Type *TSharedPtr<Type>::operator->() const noexcept
{
    assert( IsValid() );
    return Object;
}

template <typename Type> Type *TSharedPtr<Type>::Get () const noexcept
{
    return Object;
}

template <typename Type> Bool TSharedPtr<Type>::IsValid () const noexcept
{
    return Object != nullptr;
}

template <typename Type> TSharedPtr<Type>::operator Bool () const noexcept
{
    return IsValid();
}

template <typename Type> void TSharedPtr<Type>::Reset () noexcept
{
    Release();
    Object     = nullptr;
    Controller = nullptr;
}

template <typename Type> void TSharedPtr<Type>::Release () noexcept
{
    if ( ( Controller != nullptr ) and Controller->SharedCount.fetch_sub( 1, std::memory_order_acq_rel ) == 1 )
    {
        Controller->DestroyObject();
        Controller->Deallocate();
    }
}

///
/// SharedPtr Builders
///

// NOLINTNEXTLINE(misc-use-anonymous-namespace)
template <typename ObjectType, typename... Arguments> TSharedRef<ObjectType> MakeShared ( Arguments &&...InArgs ) noexcept
{
    SharedPtrInternal::TIntrusiveReferenceController<ObjectType> *Controller =
        SharedPtrInternal::NewIntrusiveReferenceController<ObjectType>( std::forward<Arguments>( InArgs )... );
    return MakeSharedRef<ObjectType>( Controller->GetObjectPtr(),
                                      static_cast<SharedPtrInternal::FReferenceController *>( Controller ) );
}

template <typename ObjectType> SharedPtrInternal::TRawPtrProxy<ObjectType> MakeShareable ( ObjectType *InObject ) noexcept
{
    return SharedPtrInternal::TRawPtrProxy<ObjectType>( InObject );
}

///
/// Static Casts
///

// NOLINTNEXTLINE(misc-use-anonymous-namespace)
template <typename CastToType, typename CastFromType>
TSharedPtr<CastToType> StaticCastSharedPtr ( const TSharedPtr<CastFromType> &InSharedPtr ) noexcept
{
    if ( InSharedPtr.IsValid() )
    {
        InSharedPtr.Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
        return MakeSharedRef<CastToType>( static_cast<CastToType *>( InSharedPtr.Object ), InSharedPtr.Controller );
    }

    return TSharedPtr<CastToType>();
}
} // namespace Lumen
