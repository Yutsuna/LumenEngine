/**
 * @file ForEach.inl
 * @brief Inline implementation of ForEach algorithm.
 */

#pragma once

#include <Algo/ForEach.hpp>

#include <utility>

/**
 * Public
 */

template <typename TObject, typename TPredicate, typename TFunc>
    requires( Lumen::Concepts::CConvertibleTo<Lumen::TInvokeResultT<TPredicate, Lumen::Algo::Internal::TRangeReferenceT<TObject>>,
                                              Lumen::Bool> )
constexpr void Lumen::Algo::ForEach ( TObject &&Object, TPredicate &&Predicate, TFunc &&Func )
{
    /**
     * INFO: Bind the callable once to avoid forwarding an rvalue callable on every iteration.
     *       Otherwise it would move-from it repeatedly.
     */
    auto &&Pred     = std::forward<TPredicate>( Predicate );
    auto &&Callable = std::forward<TFunc>( Func );

    for ( auto &&Item : Object )
    {
        if ( Invoke( Pred, Item ) )
        {
            Invoke( Callable, Item );
        }
    }
}

template <typename TObject, typename TFunc> constexpr void Lumen::Algo::ForEach ( TObject &&Object, TFunc &&Func )
{
    auto &&Callable = std::forward<TFunc>( Func );

    for ( auto &&Item : Object )
    {
        Invoke( Callable, Item );
    }
}

template <typename TObject, typename TFunc>
    requires( Lumen::Concepts::CConvertibleTo<Lumen::TInvokeResultT<TFunc, Lumen::Algo::Internal::TRangeReferenceT<TObject>>,
                                              Lumen::Bool> )
constexpr Lumen::Algo::Internal::TRangeElementPtr<TObject> Lumen::Algo::ForEachUntil ( TObject &&Object, TFunc &&Func )
{
    auto &&Callable = std::forward<TFunc>( Func );

    for ( auto &&Item : Object )
    {
        if ( Invoke( Callable, Item ) )
        {
            return &Item;
        }
    }
    return nullptr;
}

template <typename TObject, typename TPredicate, typename TFunc>
    requires( Lumen::Concepts::CConvertibleTo<Lumen::TInvokeResultT<TPredicate, Lumen::Algo::Internal::TRangeReferenceT<TObject>>,
                                              Lumen::Bool> &&
              Lumen::Concepts::CConvertibleTo<Lumen::TInvokeResultT<TFunc, Lumen::Algo::Internal::TRangeReferenceT<TObject>>,
                                              Lumen::Bool> )
constexpr Lumen::Bool Lumen::Algo::ForEachUntil ( TObject &&Object, TPredicate &&Predicate, TFunc &&Func )
{
    auto &&Pred     = std::forward<TPredicate>( Predicate );
    auto &&Callable = std::forward<TFunc>( Func );

    for ( auto &&Item : Object )
    {
        if ( Invoke( Pred, Item ) && Invoke( Callable, Item ) )
        {
            return true;
        }
    }
    return false;
}
