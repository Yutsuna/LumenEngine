/**
 * @file SharedPtrInternal.inl
 * @brief Inline implementations for Shared Pointer Internal classes.
 */

#pragma once

#include "Container/SharedPtr.hpp"

namespace LumenEngine
{

namespace SharedPtrInternal
{

    template <typename ObjectType, typename... Arguments>
    SharedPtrInternal::TIntrusiveReferenceController<ObjectType> *NewIntrusiveReferenceController ( Arguments &&...InArgs )
    {
        return new SharedPtrInternal::TIntrusiveReferenceController<ObjectType>( std::forward<Arguments>( InArgs )... );
    }

    template <typename ObjectType> SharedPtrInternal::TDefaultReferenceController<ObjectType> *NewDefaultReferenceController ( ObjectType *InObject )
    {
        return new SharedPtrInternal::TDefaultReferenceController<ObjectType>( InObject );
    }

    /**
     * TIntrusiveReferenceController
     */

    template <typename Type> template <typename... Arguments> TIntrusiveReferenceController<Type>::TIntrusiveReferenceController ( Arguments &&...InArgs )
    {
        new ( Storage ) Type( std::forward<Arguments>( InArgs )... );
    }

    template <typename Type> void TIntrusiveReferenceController<Type>::DestroyObject ()
    {
        reinterpret_cast<Type *>( Storage )->~Type();
    }

    template <typename Type> void TIntrusiveReferenceController<Type>::Deallocate ()
    {
        delete this;
    }

    template <typename Type> Type *TIntrusiveReferenceController<Type>::GetObjectPtr ()
    {
        return reinterpret_cast<Type *>( Storage );
    }

    /**
     * TDefaultReferenceController
     */

    template <typename Type> TDefaultReferenceController<Type>::TDefaultReferenceController ( Type *InObject ) : Object( InObject )
    {
        /* Empty */
    }

    template <typename Type> void TDefaultReferenceController<Type>::DestroyObject ()
    {
        delete Object;
    }

    template <typename Type> void TDefaultReferenceController<Type>::Deallocate ()
    {
        delete this;
    }

    /**
     * TRawPtrProxy
     */
    template <typename Type> TRawPtrProxy<Type>::TRawPtrProxy ( Type *InObject ) : Object( InObject )
    {
        /* Empty */
    }

} // namespace SharedPtrInternal

} // namespace LumenEngine
