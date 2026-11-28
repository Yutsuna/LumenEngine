/**
 * @file SharedRef.inl
 * @brief Inline implementations for Shared Reference classes.
 */

#pragma once

#include <Memory/SharedPtr.hpp>

namespace Lumen
{

template <typename Type>
TSharedRef<Type>::TSharedRef ( const TSharedRef &Other ) noexcept : Object( Other.Object ), Controller( Other.Controller )
{
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
}

template <typename Type>
TSharedRef<Type>::TSharedRef ( TSharedRef &&Other ) noexcept : Object( Other.Object ), Controller( Other.Controller )
{
    Other.Controller = nullptr;
}

template <typename Type>
template <typename OtherType>
    requires Concepts::CConvertibleTo<OtherType *, Type *>
TSharedRef<Type>::TSharedRef( const Internal::TRawPtrProxy<OtherType> &Proxy ) noexcept
    /** NOLINTNEXTLINE(bugprone-unhandled-exception-at-new) */
    : Object( Proxy.Object ), Controller( new Internal::TDefaultReferenceController<OtherType>( Proxy.Object ) )
{
    LUMEN_ASSERT( Object != nullptr );
}

template <typename Type>
template <typename OtherType>
    requires Concepts::CConvertibleTo<OtherType *, Type *>
TSharedRef<Type>::TSharedRef( const TSharedRef<OtherType> &Other ) noexcept
    : Object( Other.Object ), Controller( Other.Controller )
{
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
}

template <typename Type> TSharedRef<Type>::~TSharedRef () noexcept
{
    Release();
}

template <typename Type> TSharedRef<Type> &TSharedRef<Type>::operator=( const TSharedRef &Other ) noexcept
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

template <typename Type> TSharedRef<Type> &TSharedRef<Type>::operator=( TSharedRef &&Other ) noexcept
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

template <typename Type> Type &TSharedRef<Type>::operator*() const noexcept
{
    return *Object;
}

template <typename Type> Type *TSharedRef<Type>::operator->() const noexcept
{
    return Object;
}

template <typename Type>
template <typename OtherType>
Bool TSharedPtr<Type>::operator==( const TSharedRef<OtherType> &Other ) const noexcept
{
    return Object == Other.Get();
}

template <typename Type>
template <typename OtherType>
Bool TSharedPtr<Type>::operator!=( const TSharedRef<OtherType> &Other ) const noexcept
{
    return Object != Other.Get();
}

template <typename Type>
template <typename OtherType>
Bool TSharedPtr<Type>::operator==( const TSharedPtr<OtherType> &Other ) const noexcept
{
    return Object == Other.Object;
}

template <typename Type>
template <typename OtherType>
Bool TSharedPtr<Type>::operator!=( const TSharedPtr<OtherType> &Other ) const noexcept
{
    return Object != Other.Object;
}

template <typename Type> Type *TSharedRef<Type>::Get () const noexcept
{
    return Object;
}

template <typename Type> UInt64 TSharedRef<Type>::GetSharedReferenceCount () const noexcept
{
    return Controller->SharedCount.load( std::memory_order_relaxed );
}

template <typename Type>
TSharedRef<Type>::TSharedRef ( Type *InObject, Internal::FReferenceController *InController ) noexcept
    : Object( InObject ), Controller( InController )
{
    LUMEN_ASSERT( InObject != nullptr );
}

template <typename Type> void TSharedRef<Type>::Release () noexcept
{
    if ( ( Controller != nullptr ) and Controller->SharedCount.fetch_sub( 1, std::memory_order_acq_rel ) == 1 )
    {
        Controller->DestroyObject();
        Controller->Deallocate();
    }
}

/**
 * SharedRef Builder
 */

template <typename ObjectType>
/** NOLINTNEXTLINE(misc-use-anonymous-namespace) */
TSharedRef<ObjectType> MakeSharedRef ( ObjectType *InObject, Internal::FReferenceController *InController ) noexcept
{
    return TSharedRef<ObjectType>( InObject, InController );
}

/**
 * StaticCast
 */

template <typename CastToType, typename CastFromType>
/** NOLINTNEXTLINE(misc-use-anonymous-namespace,misc-use-internal-linkage) */
TSharedRef<CastToType> StaticCastSharedRef ( TSharedRef<CastFromType> const &InSharedRef ) noexcept
{
    InSharedRef.Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
    return MakeSharedRef<CastToType>( static_cast<CastToType *>( InSharedRef.Object ), InSharedRef.Controller );
}

} // namespace Lumen
