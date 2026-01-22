/**
 * @file SharedPtr.inl
 * @brief Inline implementations for Shared Pointer classes.
 */

#pragma once

#include "Container/SharedPtr.hpp"

namespace LumenEngine
{

template <typename Type>
TSharedPtr<Type>::TSharedPtr()
    : Object( nullptr ), Controller( nullptr )
{
}

template <typename Type>
template <typename OtherType, typename>
TSharedPtr<Type>::TSharedPtr( const TSharedRef<OtherType> &Other )
    : Object( Other.Object ), Controller( Other.Controller )
{
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
}

template <typename Type>
TSharedPtr<Type>::TSharedPtr( const TSharedPtr &Other )
    : Object( Other.Object ), Controller( Other.Controller )
{
    if ( Controller )
    {
        Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
    }
}

template <typename Type>
TSharedPtr<Type>::TSharedPtr( TSharedPtr &&Other )
    : Object( Other.Object ), Controller( Other.Controller )
{
    Other.Object     = nullptr;
    Other.Controller = nullptr;
}

template <typename Type>
TSharedPtr<Type>::~TSharedPtr()
{
    Release();
}

template <typename Type>
TSharedPtr<Type> &TSharedPtr<Type>::operator=( const TSharedPtr &Other )
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

template <typename Type>
TSharedPtr<Type> &TSharedPtr<Type>::operator=( const TSharedRef<Type> &Other )
{
    Release();
    Object     = Other.Object;
    Controller = Other.Controller;
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
    return *this;
}

template <typename Type>
Type &TSharedPtr<Type>::operator*() const
{
    assert( IsValid() );
    return *Object;
}

template <typename Type>
Type *TSharedPtr<Type>::operator->() const
{
    assert( IsValid() );
    return Object;
}

template <typename Type>
Type *TSharedPtr<Type>::Get () const
{
    return Object;
}

template <typename Type>
bool TSharedPtr<Type>::IsValid () const
{
    return Object != nullptr;
}

template <typename Type>
TSharedPtr<Type>::operator bool () const
{
    return IsValid();
}

template <typename Type>
void TSharedPtr<Type>::Reset ()
{
    Release();
    Object     = nullptr;
    Controller = nullptr;
}

template <typename Type>
void TSharedPtr<Type>::Release ()
{
    if ( Controller && Controller->SharedCount.fetch_sub( 1, std::memory_order_acq_rel ) == 1 )
    {
        Controller->DestroyObject();
        Controller->Deallocate();
    }
}

namespace
{

    /**
     * @brief Creates a new TIntrusiveReferenceController instance with the given arguments.
     * @param InArgs Arguments to forward to the constructor of ObjectType.
     * @return A pointer to the newly created TIntrusiveReferenceController.
     */
    template <typename ObjectType, typename... Arguments>
    static inline SharedPtrInternal::TIntrusiveReferenceController<ObjectType> *NewIntrusiveReferenceController ( Arguments &&...InArgs )
    {
        return new SharedPtrInternal::TIntrusiveReferenceController<ObjectType>( std::forward<Arguments>( InArgs )... );
    }

} // namespace

template <typename ObjectType>
static inline TSharedRef<ObjectType> MakeSharedRef ( ObjectType *InObject, SharedPtrInternal::FReferenceController *InController )
{
    return TSharedRef<ObjectType>( InObject, InController );
}

template <typename ObjectType, typename... Arguments>
static inline TSharedRef<ObjectType> MakeShared ( Arguments &&...InArgs )
{
    SharedPtrInternal::TIntrusiveReferenceController<ObjectType> *Controller = NewIntrusiveReferenceController<ObjectType>( std::forward<Arguments>( InArgs )... );
    return MakeSharedRef<ObjectType>( Controller->GetObjectPtr(), static_cast<SharedPtrInternal::FReferenceController *>( Controller ) );
}

} // namespace LumenEngine
