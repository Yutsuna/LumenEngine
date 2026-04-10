/**
 * @file UniquePtr.inl
 * @brief Inline implementations for Unique Pointer class.
 */

#pragma once

#include "Container/UniquePtr.hpp"

namespace LumenEngine
{

template <typename Type, typename Deleter> TUniquePtr<Type, Deleter>::TUniquePtr () : Object( nullptr ), DeleterInstance()
{
    /* Empty */
}

template <typename Type, typename Deleter> TUniquePtr<Type, Deleter>::TUniquePtr ( NullptrType ) : Object( nullptr ), DeleterInstance()
{
    /* Empty */
}

template <typename Type, typename Deleter> TUniquePtr<Type, Deleter>::TUniquePtr ( Type *InObject ) : Object( InObject ), DeleterInstance()
{
    /* Empty */
}

template <typename Type, typename Deleter>
TUniquePtr<Type, Deleter>::TUniquePtr ( Type *InObject, Deleter InDeleter ) : Object( InObject ), DeleterInstance( std::move( InDeleter ) )
{
    /* Empty */
}

template <typename Type, typename Deleter>
TUniquePtr<Type, Deleter>::TUniquePtr ( TUniquePtr &&Other ) noexcept : Object( Other.Object ), DeleterInstance( std::move( Other.DeleterInstance ) )
{
    Other.Object = nullptr;
}

template <typename Type, typename Deleter>
template <typename OtherType, typename OtherDeleter>
    requires Concepts::CConvertibleTo<OtherType *, Type *>
TUniquePtr<Type, Deleter>::TUniquePtr( TUniquePtr<OtherType, OtherDeleter> &&Other ) noexcept
    : Object( Other.Object ), DeleterInstance( std::move( Other.DeleterInstance ) )
{
    Other.Object = nullptr;
}

template <typename Type, typename Deleter> TUniquePtr<Type, Deleter>::~TUniquePtr ()
{
    Reset();
}

template <typename Type, typename Deleter> TUniquePtr<Type, Deleter> &TUniquePtr<Type, Deleter>::operator=( TUniquePtr &&Other ) noexcept
{
    if ( this != &Other )
    {
        Reset( Other.Release() );
        DeleterInstance = std::move( Other.DeleterInstance );
    }
    return *this;
}

template <typename Type, typename Deleter>
template <typename OtherType, typename OtherDeleter>
    requires Concepts::CConvertibleTo<OtherType *, Type *>
TUniquePtr<Type, Deleter> &TUniquePtr<Type, Deleter>::operator=( TUniquePtr<OtherType, OtherDeleter> &&Other ) noexcept
{
    Reset( Other.Release() );
    DeleterInstance = std::move( Other.DeleterInstance );
    return *this;
}

template <typename Type, typename Deleter> TUniquePtr<Type, Deleter> &TUniquePtr<Type, Deleter>::operator=( NullptrType ) noexcept
{
    Reset();
    return *this;
}

template <typename Type, typename Deleter> Type &TUniquePtr<Type, Deleter>::operator*() const
{
    assert( IsValid() );
    return *Object;
}

template <typename Type, typename Deleter> Type *TUniquePtr<Type, Deleter>::operator->() const
{
    assert( IsValid() );
    return Object;
}

template <typename Type, typename Deleter> Type *TUniquePtr<Type, Deleter>::Get () const
{
    return Object;
}

template <typename Type, typename Deleter> Deleter &TUniquePtr<Type, Deleter>::GetDeleter ()
{
    return DeleterInstance;
}

template <typename Type, typename Deleter> const Deleter &TUniquePtr<Type, Deleter>::GetDeleter () const
{
    return DeleterInstance;
}

template <typename Type, typename Deleter> bool TUniquePtr<Type, Deleter>::IsValid () const
{
    return Object != nullptr;
}

template <typename Type, typename Deleter> TUniquePtr<Type, Deleter>::operator bool () const
{
    return IsValid();
}

template <typename Type, typename Deleter> Type *TUniquePtr<Type, Deleter>::Release ()
{
    Type *TempObject = Object;
    Object           = nullptr;
    return TempObject;
}

template <typename Type, typename Deleter> void TUniquePtr<Type, Deleter>::Reset ( Type *InObject )
{
    Type *OldObject = Object;
    Object          = InObject;

    if ( OldObject )
    {
        DeleterInstance( OldObject );
    }
}

/**
 * TUniquePtr<Type[]> implementation
 */

template <typename Type, typename Deleter> TUniquePtr<Type[], Deleter>::TUniquePtr () : Object( nullptr ), DeleterInstance()
{
    /* Empty */
}

template <typename Type, typename Deleter> TUniquePtr<Type[], Deleter>::TUniquePtr ( NullptrType ) : Object( nullptr ), DeleterInstance()
{
    /* Empty */
}

template <typename Type, typename Deleter> TUniquePtr<Type[], Deleter>::TUniquePtr ( Type *InObject ) : Object( InObject ), DeleterInstance()
{
    /* Empty */
}

template <typename Type, typename Deleter>
TUniquePtr<Type[], Deleter>::TUniquePtr ( TUniquePtr &&Other ) noexcept : Object( Other.Object ), DeleterInstance( std::move( Other.DeleterInstance ) )
{
    Other.Object = nullptr;
}

template <typename Type, typename Deleter> TUniquePtr<Type[], Deleter>::~TUniquePtr ()
{
    Reset();
}

template <typename Type, typename Deleter> TUniquePtr<Type[], Deleter> &TUniquePtr<Type[], Deleter>::operator=( TUniquePtr &&Other ) noexcept
{
    if ( this != &Other )
    {
        Reset( Other.Release() );
        DeleterInstance = std::move( Other.DeleterInstance );
    }
    return *this;
}

template <typename Type, typename Deleter> TUniquePtr<Type[], Deleter> &TUniquePtr<Type[], Deleter>::operator=( NullptrType ) noexcept
{
    Reset();
    return *this;
}

template <typename Type, typename Deleter> Type &TUniquePtr<Type[], Deleter>::operator[]( USize Index ) const
{
    assert( IsValid() );
    return Object[Index];
}

template <typename Type, typename Deleter> Type *TUniquePtr<Type[], Deleter>::Get () const
{
    return Object;
}

template <typename Type, typename Deleter> Deleter &TUniquePtr<Type[], Deleter>::GetDeleter ()
{
    return DeleterInstance;
}

template <typename Type, typename Deleter> const Deleter &TUniquePtr<Type[], Deleter>::GetDeleter () const
{
    return DeleterInstance;
}

template <typename Type, typename Deleter> bool TUniquePtr<Type[], Deleter>::IsValid () const
{
    return Object != nullptr;
}

template <typename Type, typename Deleter> TUniquePtr<Type[], Deleter>::operator bool () const
{
    return IsValid();
}

template <typename Type, typename Deleter> Type *TUniquePtr<Type[], Deleter>::Release ()
{
    Type *TempObject = Object;
    Object           = nullptr;
    return TempObject;
}

template <typename Type, typename Deleter> void TUniquePtr<Type[], Deleter>::Reset ( Type *InObject )
{
    Type *OldObject = Object;
    Object          = InObject;

    if ( OldObject )
    {
        DeleterInstance( OldObject );
    }
}

/**
 * UniquePtr Builder
 */

template <typename ObjectType, typename... Arguments>
    requires( !std::is_array_v<ObjectType> )
static inline TUniquePtr<ObjectType> MakeUnique ( Arguments &&...InArgs )
{
    return TUniquePtr<ObjectType>( new ObjectType( std::forward<Arguments>( InArgs )... ) );
}

template <typename ObjectType>
    requires std::is_array_v<ObjectType>
static inline TUniquePtr<ObjectType> MakeUnique ( USize Size )
{
    using ElementType = std::remove_extent_t<ObjectType>;
    return TUniquePtr<ObjectType>( new ElementType[Size]() );
}

} // namespace LumenEngine
