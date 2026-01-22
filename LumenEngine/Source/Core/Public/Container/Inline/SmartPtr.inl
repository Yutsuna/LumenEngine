#pragma once

#include "Container/SmartPtr.hpp"

namespace LumenEngine
{

/**
 * TIntrusiveReferenceController
 */

template <typename Type>
template <typename... Arguments>
SharedPtrInternal::TIntrusiveReferenceController<Type>::TIntrusiveReferenceController( Arguments &&...InArgs )
{
    new ( Storage ) Type( std::forward<Arguments>( InArgs )... );
}

template <typename Type>
void SharedPtrInternal::TIntrusiveReferenceController<Type>::DestroyObject ()
{
    reinterpret_cast<Type *>( Storage )->~Type();
}

template <typename Type>
void SharedPtrInternal::TIntrusiveReferenceController<Type>::Deallocate ()
{
    delete this;
}

template <typename Type>
Type *SharedPtrInternal::TIntrusiveReferenceController<Type>::GetObjectPtr ()
{
    return reinterpret_cast<Type *>( Storage );
}

/**
 * TSharedRef
 */

template <typename Type>
TSharedRef<Type>::TSharedRef( const TSharedRef &Other )
    : Object( Other.Object ), Controller( Other.Controller )
{
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
}

template <typename Type>
TSharedRef<Type>::TSharedRef( TSharedRef &&Other )
    : Object( Other.Object ), Controller( Other.Controller )
{
    /* __empty__ */
}

template <typename Type>
template <typename OtherType, typename>
TSharedRef<Type>::TSharedRef( const TSharedRef<OtherType> &Other )
    : Object( Other.Object ), Controller( Other.Controller )
{
    Controller->SharedCount.fetch_add( 1, std::memory_order_relaxed );
}

template <typename Type>
TSharedRef<Type>::~TSharedRef()
{
    Release();
}

template <typename Type>
TSharedRef<Type> &TSharedRef<Type>::operator=( const TSharedRef &Other )
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

template <typename Type>
Type &TSharedRef<Type>::operator*() const
{
    return *Object;
}

template <typename Type>
Type *TSharedRef<Type>::operator->() const
{
    return Object;
}

template <typename Type>
Type *TSharedRef<Type>::Get () const
{
    return Object;
}

template <typename Type>
Int32 TSharedRef<Type>::GetSharedReferenceCount () const
{
    return Controller->SharedCount.load( std::memory_order_relaxed );
}

template <typename Type>
TSharedRef<Type>::TSharedRef( Type *InObject, SharedPtrInternal::FReferenceController *InController )
    : Object( InObject ), Controller( InController )
{
    assert( InObject != nullptr );
}

template <typename Type>
void TSharedRef<Type>::Release ()
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
static inline TSharedRef<ObjectType> MakeSharedRef ( ObjectType *InObject, SharedPtrInternal::FReferenceController *InSharedReferenceCount )
{
    return TSharedRef<ObjectType>( InObject, InSharedReferenceCount );
}

template <typename ObjectType, typename... Arguments>
static inline TSharedRef<ObjectType> MakeShared ( Arguments &&...InArgs )
{
    SharedPtrInternal::TIntrusiveReferenceController<ObjectType> *Controller = NewIntrusiveReferenceController<ObjectType>( std::forward<Arguments>( InArgs )... );
    return MakeShared<ObjectType>( Controller->GetObjectPtr(), static_cast<SharedPtrInternal::FReferenceController *>( Controller ) );
}

} // namespace LumenEngine
