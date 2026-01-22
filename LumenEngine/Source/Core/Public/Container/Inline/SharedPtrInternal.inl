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

    template <typename Type>
    template <typename... Arguments>
    TIntrusiveReferenceController<Type>::TIntrusiveReferenceController( Arguments &&...InArgs )
    {
        new ( Storage ) Type( std::forward<Arguments>( InArgs )... );
    }

    template <typename Type>
    void TIntrusiveReferenceController<Type>::DestroyObject ()
    {
        reinterpret_cast<Type *>( Storage )->~Type();
    }

    template <typename Type>
    void TIntrusiveReferenceController<Type>::Deallocate ()
    {
        delete this;
    }

    template <typename Type>
    Type *TIntrusiveReferenceController<Type>::GetObjectPtr ()
    {
        return reinterpret_cast<Type *>( Storage );
    }

} // namespace SharedPtrInternal

} // namespace LumenEngine
