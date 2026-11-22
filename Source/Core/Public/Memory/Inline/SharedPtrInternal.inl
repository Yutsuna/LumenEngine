/**
 * @file SharedPtrInternal.inl
 * @brief Inline implementations for Shared Pointer Internal classes.
 */

#pragma once

#include <Memory/SharedPtr.hpp>

namespace Lumen
{

namespace Internal
{
    template <typename ObjectType, typename... Arguments>
    Internal::TIntrusiveReferenceController<ObjectType> *NewIntrusiveReferenceController ( Arguments &&...InArgs ) noexcept
    {
        /** NOLINTNEXTLINE(bugprone-unhandled-exception-at-new) */
        return new Internal::TIntrusiveReferenceController<ObjectType>( std::forward<Arguments>( InArgs )... );
    }

    template <typename ObjectType>
    Internal::TDefaultReferenceController<ObjectType> *NewDefaultReferenceController ( ObjectType *InObject ) noexcept
    {
        /** NOLINTNEXTLINE(bugprone-unhandled-exception-at-new) */
        return new Internal::TDefaultReferenceController<ObjectType>( InObject );
    }

    /**
     * TIntrusiveReferenceController
     */

    template <typename Type>
    template <typename... Arguments>
    TIntrusiveReferenceController<Type>::TIntrusiveReferenceController( Arguments &&...InArgs ) noexcept
    {
        new ( Storage ) Type( std::forward<Arguments>( InArgs )... );
    }

    template <typename Type> void TIntrusiveReferenceController<Type>::DestroyObject () noexcept
    {
        reinterpret_cast<Type *>( Storage )->~Type();
    }

    template <typename Type> void TIntrusiveReferenceController<Type>::Deallocate () noexcept
    {
        delete this;
    }

    template <typename Type> Type *TIntrusiveReferenceController<Type>::GetObjectPtr () noexcept
    {
        return reinterpret_cast<Type *>( Storage );
    }

    /**
     * TDefaultReferenceController
     */

    template <typename Type>
    TDefaultReferenceController<Type>::TDefaultReferenceController( Type *InObject ) noexcept : Object( InObject )
    {
        /** Empty */
    }

    template <typename Type> void TDefaultReferenceController<Type>::DestroyObject () noexcept
    {
        delete Object;
    }

    template <typename Type> void TDefaultReferenceController<Type>::Deallocate () noexcept
    {
        delete this;
    }

    /**
     * TRawPtrProxy
     */
    template <typename Type> TRawPtrProxy<Type>::TRawPtrProxy( Type *InObject ) noexcept : Object( InObject )
    {
        /** Empty */
    }

} // namespace Internal

} // namespace Lumen
