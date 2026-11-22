/**
 * @file Any.inl
 * @brief Inline implementation of AnyOf algorithm.
 */

#pragma once

#include <Algo/Any.hpp>

#include <utility>

/**
 * Public
 */

template <typename TObject, typename TPredicate>
    requires( Lumen::Concepts::CConvertibleTo<Lumen::TInvokeResultT<TPredicate, Lumen::Algo::Internal::TRangeReferenceT<TObject>>,
                                              Lumen::Bool> )
constexpr Lumen::Bool Lumen::Algo::AnyOf ( TObject &&Object, TPredicate &&Predicate )
{
    auto &&Pred = std::forward<TPredicate>( Predicate );

    /** NOLINTNEXTLINE(readability-use-anyofallof) */
    for ( auto &&Item : Object )
    {
        if ( Invoke( Pred, Item ) )
        {
            return true;
        }
    }

    return false;
}
