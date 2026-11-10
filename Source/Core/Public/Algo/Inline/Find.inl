/**
 * @file Find.inl
 * @brief Find algorithms over ranges implementation.
 */

#pragma once

#include <Algo/Find.hpp>
#include <Algo/ForEach.hpp>

#include <Templates/Invoke.hpp>

/**
 * Iterator
 */

template <class TIterator, class TPredicate>
constexpr TIterator Lumen::Algo::FindIf ( TIterator InFirst, TIterator InLast, TPredicate &&InPredicate )
{
    for ( ; InFirst != InLast; ++InFirst )
    {
        if ( Invoke( std::forward<TPredicate>( InPredicate ), *InFirst ) )
        {
            return InFirst;
        }
    }
    return InLast;
}

template <class TIterator, class TValue>
constexpr TIterator Lumen::Algo::Find ( TIterator InFirst, TIterator InLast, const TValue &InValue )
{
    return FindIf( InFirst, InLast, [&] ( auto &&Item ) -> Bool
    {
        return Item == InValue;
    } );
}

template <class TIterator, class TPredicate>
constexpr TIterator Lumen::Algo::FindIfNot ( TIterator InFirst, TIterator InLast, TPredicate &&InPredicate )
{
    return FindIf( InFirst, InLast, [&] ( auto &&Item ) -> Bool
    {
        return not Invoke( InPredicate, Item );
    } );
}

template <class TIterator, class TValue, class TComparator>
constexpr TIterator
Lumen::Algo::FindIfEqual ( TIterator InFirst, TIterator InLast, const TValue &InValue, TComparator &&InComparator )
{
    return FindIf( InFirst, InLast, [&] ( auto &&Item ) -> Bool
    {
        return Invoke( InComparator, Item, InValue );
    } );
}

/**
 * TOptional<Iterator>
 */

template <class TIterator, class TPredicate>
constexpr Lumen::TOptional<TIterator> Lumen::Algo::FindIfOrNone ( TIterator InFirst, TIterator InLast, TPredicate &&InPredicate )
{
    TIterator Result = FindIf( InFirst, InLast, std::forward<TPredicate>( InPredicate ) );
    if ( Result != InLast )
    {
        return TOptional<TIterator>( Result );
    }
    return TOptional<TIterator>();
}

template <class TIterator, class TValue>
constexpr Lumen::TOptional<TIterator> Lumen::Algo::FindOrNone ( TIterator InFirst, TIterator InLast, const TValue &InValue )
{
    return FindIfOrNone( InFirst, InLast, [&] ( auto &&Item ) -> Bool
    {
        return Item == InValue;
    } );
}

template <class TIterator, class TPredicate>
constexpr Lumen::TOptional<TIterator>
Lumen::Algo::FindIfNotOrNone ( TIterator InFirst, TIterator InLast, TPredicate &&InPredicate )
{
    return FindIfOrNone( InFirst, InLast, [&] ( auto &&Item ) -> Bool
    {
        return not Invoke( InPredicate, Item );
    } );
}

template <class TIterator, class TValue, class TComparator>
constexpr Lumen::TOptional<TIterator>
Lumen::Algo::FindIfEqualOrNone ( TIterator InFirst, TIterator InLast, const TValue &InValue, TComparator &&InComparator )
{
    return FindIfOrNone( InFirst, InLast, [&] ( auto &&Item ) -> Bool
    {
        return Invoke( InComparator, Item, InValue );
    } );
}

/**
 * TPtr
 */

template <class TRangeType, class TPredicate>
constexpr Lumen::Algo::Internal::TRangeElementPtr<TRangeType> Lumen::Algo::FindIf ( TRangeType &&InRange,
                                                                                    TPredicate &&InPredicate )
{
    return ForEachUntil( InRange, std::forward<TPredicate>( InPredicate ) );
}

template <class TRangeType, class TValue>
constexpr Lumen::Algo::Internal::TRangeElementPtr<TRangeType> Lumen::Algo::Find ( TRangeType &&InRange, const TValue &InValue )
{
    for ( auto &&Elem : InRange )
    {
        if ( Elem == InValue )
        {
            return &Elem;
        }
    }
    return {};
}

template <class TRangeType, class TPredicate>
constexpr Lumen::Algo::Internal::TRangeElementPtr<TRangeType> Lumen::Algo::FindIfNot ( TRangeType &&InRange,
                                                                                       TPredicate &&InPredicate )
{
    for ( auto &&Elem : InRange )
    {
        if ( not Invoke( std::forward<TPredicate>( InPredicate ), Elem ) )
        {
            return &Elem;
        }
    }
    return {};
}

/**
 * TOptional<Value>
 */

template <class TRangeType, class TPredicate>
constexpr Lumen::TOptional<Lumen::Algo::Internal::TRangeElementValue<TRangeType>>
Lumen::Algo::FindIfOrNone ( TRangeType &&InRange, TPredicate &&InPredicate )
{
    using TElement = Internal::TRangeElementValue<TRangeType>;

    if ( auto *Result = FindIf( std::forward<TRangeType>( InRange ), std::forward<TPredicate>( InPredicate ) ) )
    {
        return TOptional<TElement>( *Result );
    }
    return TOptional<TElement>();
}

template <class TRangeType, class TValue>
constexpr Lumen::TOptional<Lumen::Algo::Internal::TRangeElementValue<TRangeType>>
Lumen::Algo::FindOrNone ( TRangeType &&InRange, const TValue &InValue )
{
    using TElement = Internal::TRangeElementValue<TRangeType>;

    if ( auto *Result = Find( std::forward<TRangeType>( InRange ), InValue ) )
    {
        return TOptional<TElement>( *Result );
    }
    return TOptional<TElement>();
}

template <class TRangeType, class TPredicate>
constexpr Lumen::TOptional<Lumen::Algo::Internal::TRangeElementValue<TRangeType>>
Lumen::Algo::FindIfNotOrNone ( TRangeType &&InRange, TPredicate &&InPredicate )
{
    using TElement = Internal::TRangeElementValue<TRangeType>;

    if ( auto *Result = FindIfNot( std::forward<TRangeType>( InRange ), std::forward<TPredicate>( InPredicate ) ) )
    {
        return TOptional<TElement>( *Result );
    }
    return TOptional<TElement>();
}
