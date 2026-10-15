/// @file UniquePtrInternal.inl
/// @brief Inline implementation of UniquePtrInternal namespace

#pragma once

#include <Memory/UniquePtr.hpp>

#include <cassert>
#include <utility>

namespace Lumen
{
namespace UniquePtrInternal
{
    template <typename Type>
    template <typename OtherType>
        requires Concepts::CConvertibleTo<OtherType *, Type *>
    TDefaultDelete<Type>::TDefaultDelete( const TDefaultDelete<OtherType> & ) noexcept
    {
        /// Empty
    }

    template <typename Type> void TDefaultDelete<Type>::operator()( const Type *Ptr ) const noexcept
    {
        static_assert( sizeof( Type ), "Cannot delete an incomplete type." );
        delete Ptr;
    }

    template <typename Type> void TDefaultDelete<Type[]>::operator()( const Type *Ptr ) const noexcept
    {
        static_assert( sizeof( Type ), "Cannot delete an incomplete type." );
        delete[] Ptr;
    }
} // namespace UniquePtrInternal

template <typename Type, typename Deleter> TUniquePtr<Type, Deleter>::TUniquePtr() noexcept : Object( nullptr ), DeleterInstance()
{
    /// Empty
}

template <typename Type, typename Deleter>
TUniquePtr<Type, Deleter>::TUniquePtr( NullPtrType ) noexcept : Object( nullptr ), DeleterInstance()
{
    /// Empty
}

template <typename Type, typename Deleter>
TUniquePtr<Type, Deleter>::TUniquePtr( Type *InObject ) noexcept : Object( InObject ), DeleterInstance()
{
    /// Empty
}

template <typename Type, typename Deleter>
TUniquePtr<Type, Deleter>::TUniquePtr( Type *InObject, Deleter InDeleter ) noexcept
    : Object( InObject ), DeleterInstance( std::move( InDeleter ) )
{
    /// Empty
}

template <typename Type, typename Deleter>
TUniquePtr<Type, Deleter>::TUniquePtr( TUniquePtr &&Other ) noexcept
    : Object( Other.Release() ), DeleterInstance( std::move( Other ).GetDeleter() )
{
    /// Empty
}

template <typename Type, typename Deleter>
template <typename OtherType, typename OtherDeleter>
    requires Concepts::CConvertibleTo<OtherType *, Type *>
TUniquePtr<Type, Deleter>::TUniquePtr( TUniquePtr<OtherType, OtherDeleter> &&Other ) noexcept
    : Object( Other.Release() ), DeleterInstance( std::move( Other.DeleterInstance ) )
{
    /// Empty
}

template <typename Type, typename Deleter> TUniquePtr<Type, Deleter>::~TUniquePtr() noexcept
{
    Reset();
}

template <typename Type, typename Deleter>
TUniquePtr<Type, Deleter> &TUniquePtr<Type, Deleter>::operator=( TUniquePtr &&Other ) noexcept
{
    if ( this != &Other )
    {
        Reset( Other.Release() );
        DeleterInstance = std::move( Other ).GetDeleter();
    }
    return *this;
}

template <typename Type, typename Deleter>
template <typename OtherType, typename OtherDeleter>
    requires Concepts::CConvertibleTo<OtherType *, Type *>
TUniquePtr<Type, Deleter> &TUniquePtr<Type, Deleter>::operator=( TUniquePtr<OtherType, OtherDeleter> &&Other ) noexcept
{
    Reset( Other.Release() );
    DeleterInstance = std::move( Other ).GetDeleter();
    return *this;
}

template <typename Type, typename Deleter> TUniquePtr<Type, Deleter> &TUniquePtr<Type, Deleter>::operator=( NullPtrType ) noexcept
{
    Reset();
    return *this;
}

template <typename Type, typename Deleter> Type &TUniquePtr<Type, Deleter>::operator*() const noexcept
{
    assert( IsValid() );
    return *Object;
}

template <typename Type, typename Deleter> Type *TUniquePtr<Type, Deleter>::operator->() const noexcept
{
    assert( IsValid() );
    return Object;
}

template <typename Type, typename Deleter> Type *TUniquePtr<Type, Deleter>::Get () const noexcept
{
    return Object;
}

template <typename Type, typename Deleter> Deleter &TUniquePtr<Type, Deleter>::GetDeleter () noexcept
{
    return DeleterInstance;
}

template <typename Type, typename Deleter> const Deleter &TUniquePtr<Type, Deleter>::GetDeleter () const noexcept
{
    return DeleterInstance;
}

template <typename Type, typename Deleter> Bool TUniquePtr<Type, Deleter>::IsValid () const noexcept
{
    return Object != nullptr;
}

template <typename Type, typename Deleter> TUniquePtr<Type, Deleter>::operator Bool () const noexcept
{
    return IsValid();
}

template <typename Type, typename Deleter> Type *TUniquePtr<Type, Deleter>::Release () noexcept
{
    return std::exchange<Type *>( Object, nullptr );
}

template <typename Type, typename Deleter> void TUniquePtr<Type, Deleter>::Reset ( Type *InObject ) noexcept
{
    Type *OldObject = Object;
    Object          = InObject;

    if ( OldObject )
    {
        DeleterInstance( OldObject );
    }
}

///
/// TUniquePtr<Type[]> implementation
///

template <typename Type, typename Deleter>
TUniquePtr<Type[], Deleter>::TUniquePtr() noexcept : Object( nullptr ), DeleterInstance()
{
    /// Empty
}

template <typename Type, typename Deleter>
TUniquePtr<Type[], Deleter>::TUniquePtr( NullPtrType ) noexcept : Object( nullptr ), DeleterInstance()
{
    /// Empty
}

template <typename Type, typename Deleter>
TUniquePtr<Type[], Deleter>::TUniquePtr( Type *InObject ) noexcept : Object( InObject ), DeleterInstance()
{
    /// Empty
}

template <typename Type, typename Deleter>
TUniquePtr<Type[], Deleter>::TUniquePtr( TUniquePtr &&Other ) noexcept
    : Object( Other.Release() ), DeleterInstance( std::move( Other ).GetDeleter() )
{
    /// Empty
}

template <typename Type, typename Deleter> TUniquePtr<Type[], Deleter>::~TUniquePtr() noexcept
{
    Reset();
}

template <typename Type, typename Deleter>
TUniquePtr<Type[], Deleter> &TUniquePtr<Type[], Deleter>::operator=( TUniquePtr &&Other ) noexcept
{
    if ( this != &Other )
    {
        Reset( Other.Release() );
        DeleterInstance = std::move( Other ).GetDeleter();
    }
    return *this;
}

template <typename Type, typename Deleter>
TUniquePtr<Type[], Deleter> &TUniquePtr<Type[], Deleter>::operator=( NullPtrType ) noexcept
{
    Reset();
    return *this;
}

template <typename Type, typename Deleter> Type &TUniquePtr<Type[], Deleter>::operator[]( const USize Index ) const noexcept
{
    assert( IsValid() );
    return Object[Index];
}

template <typename Type, typename Deleter> Type *TUniquePtr<Type[], Deleter>::Get () const noexcept
{
    return Object;
}

template <typename Type, typename Deleter> Deleter &TUniquePtr<Type[], Deleter>::GetDeleter () noexcept
{
    return DeleterInstance;
}

template <typename Type, typename Deleter> const Deleter &TUniquePtr<Type[], Deleter>::GetDeleter () const noexcept
{
    return DeleterInstance;
}

template <typename Type, typename Deleter> Bool TUniquePtr<Type[], Deleter>::IsValid () const noexcept
{
    return Object != nullptr;
}

template <typename Type, typename Deleter> TUniquePtr<Type[], Deleter>::operator Bool () const noexcept
{
    return IsValid();
}

template <typename Type, typename Deleter> Type *TUniquePtr<Type[], Deleter>::Release () noexcept
{
    return std::exchange<Type *>( Object, nullptr );
}

template <typename Type, typename Deleter> void TUniquePtr<Type[], Deleter>::Reset ( Type *InObject ) noexcept
{
    Type *OldObject = Object;
    Object          = InObject;

    if ( OldObject )
    {
        DeleterInstance( OldObject );
    }
}

///
/// UniquePtr Builder
///

template <typename ObjectType, typename... Arguments>
    requires( not std::is_array_v<ObjectType> )
TUniquePtr<ObjectType> MakeUnique ( Arguments &&...InArgs ) noexcept
{
    return TUniquePtr<ObjectType>( new ObjectType( std::forward<Arguments>( InArgs )... ) );
}

template <typename ObjectType>
    requires std::is_array_v<ObjectType>
TUniquePtr<ObjectType> MakeUnique ( const USize Size ) noexcept
{
    using ElementType = std::remove_extent_t<ObjectType>;
    return TUniquePtr<ObjectType>( new ElementType[Size]() );
}
} // namespace Lumen
