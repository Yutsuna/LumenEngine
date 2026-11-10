/**
 * @file ForEach.inl
 * @brief Inline implementation of ForEach algorithm.
 */

#pragma once

#include <Algo/ForEach.hpp>

/**
 * Public
 */

template <typename TObject, typename TPredicate, typename TFunc>
void Lumen::Algo::ForEach ( TObject &&Object, TPredicate &&Predicate, TFunc &&Func )
{
    for ( auto &&Item : Object )
    {
        if ( Invoke( Predicate, Item ) )
        {
            Invoke( Func, Item );
        }
    }
}

template <typename TObject, typename TFunc> void Lumen::Algo::ForEach ( TObject &&Object, TFunc &&Func )
{
    for ( auto &&Item : Object )
    {
        Invoke( Func, Item );
    }
}

template <typename TObject, typename TFunc> Lumen::Bool Lumen::Algo::ForEachUntil ( TObject &&Object, TFunc &&Func )
{
    for ( auto &&Item : Object )
    {
        if ( Invoke( Func, Item ) )
        {
            return true;
        }
    }
    return false;
}

template <typename TObject, typename TPredicate, typename TFunc>
Lumen::Bool Lumen::Algo::ForEachUntil ( TObject &&Object, TPredicate &&Predicate, TFunc &&Func )
{
    for ( auto &&Item : Object )
    {
        if ( Invoke( Predicate, Item ) && Invoke( Func, Item ) )
        {
            return true;
        }
    }
    return false;
}
