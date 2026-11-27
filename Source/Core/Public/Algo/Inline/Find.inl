/**
 * @file Find.inl
 * @brief Find algorithms over ranges implementation.
 */

#pragma once

#include <Algo/Find.hpp>
#include <Algo/ForEach.hpp>

#include <Templates/Invoke.hpp>

#include <Concepts/IsIntegral.hpp>

#include <memory>
#include <utility>

/**
 * Iterator
 */

template <class TIterator, class TPredicate>
constexpr TIterator Lumen::Algo::FindIf ( TIterator InFirst, TIterator InLast, TPredicate &&InPredicate )
{
    auto &&Pred = std::forward<TPredicate>( InPredicate );

    for ( ; InFirst != InLast; ++InFirst )
    {
        if ( Invoke( Pred, *InFirst ) )
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
        return TOptional<TIterator>( std::move( Result ) );
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
    requires Lumen::Algo::Internal::CPointerReturnRange<TRangeType>
constexpr Lumen::Algo::Internal::TRangeElementPtr<TRangeType> Lumen::Algo::FindIf ( TRangeType &&InRange,
                                                                                    TPredicate &&InPredicate )
{
    return ForEachUntil( std::forward<TRangeType>( InRange ), std::forward<TPredicate>( InPredicate ) );
}

template <class TRangeType, class TValue>
    requires Lumen::Algo::Internal::CPointerReturnRange<TRangeType>
constexpr Lumen::Algo::Internal::TRangeElementPtr<TRangeType> Lumen::Algo::Find ( TRangeType &&InRange, const TValue &InValue )
{
    if constexpr ( Concepts::CCommonRange<TRangeType> )
    {
        auto Last   = End( InRange );
        auto Result = Find( Begin( InRange ), Last, InValue );
        return Result == Last ? nullptr : std::addressof( *Result );
    }
    else
    {
        for ( auto &&Elem : InRange )
        {
            if ( Elem == InValue )
            {
                return std::addressof( Elem );
            }
        }
        return {};
    }
}

template <class TRangeType, class TPredicate>
    requires Lumen::Algo::Internal::CPointerReturnRange<TRangeType>
constexpr Lumen::Algo::Internal::TRangeElementPtr<TRangeType> Lumen::Algo::FindIfNot ( TRangeType &&InRange,
                                                                                       TPredicate &&InPredicate )
{
    auto &&Pred = std::forward<TPredicate>( InPredicate );

    for ( auto &&Elem : InRange )
    {
        if ( not Invoke( Pred, Elem ) )
        {
            return std::addressof( Elem );
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

    if constexpr ( Internal::CPointerReturnRange<TRangeType> )
    {
        if ( auto *Result = FindIf( std::forward<TRangeType>( InRange ), std::forward<TPredicate>( InPredicate ) ) )
        {
            return TOptional<TElement>( *Result );
        }
    }
    else
    {
        auto Last   = End( InRange );
        auto Result = FindIf( Begin( InRange ), Last, std::forward<TPredicate>( InPredicate ) );
        if ( Result != Last )
        {
            if constexpr ( Internal::bConsumeRangeElements<TRangeType> )
            {
                return TOptional<TElement>( IterMove( Result ) );
            }
            else
            {
                return TOptional<TElement>( *Result );
            }
        }
    }
    return TOptional<TElement>();
}

template <class TRangeType, class TValue>
constexpr Lumen::TOptional<Lumen::Algo::Internal::TRangeElementValue<TRangeType>>
Lumen::Algo::FindOrNone ( TRangeType &&InRange, const TValue &InValue )
{
    using TElement = Internal::TRangeElementValue<TRangeType>;

    if constexpr ( Internal::CPointerReturnRange<TRangeType> )
    {
        if ( auto *Result = Find( std::forward<TRangeType>( InRange ), InValue ) )
        {
            return TOptional<TElement>( *Result );
        }
    }
    else
    {
        auto Last   = End( InRange );
        auto Result = Find( Begin( InRange ), Last, InValue );
        if ( Result != Last )
        {
            if constexpr ( Internal::bConsumeRangeElements<TRangeType> )
            {
                return TOptional<TElement>( IterMove( Result ) );
            }
            else
            {
                return TOptional<TElement>( *Result );
            }
        }
    }
    return TOptional<TElement>();
}

template <class TRangeType, class TPredicate>
constexpr Lumen::TOptional<Lumen::Algo::Internal::TRangeElementValue<TRangeType>>
Lumen::Algo::FindIfNotOrNone ( TRangeType &&InRange, TPredicate &&InPredicate )
{
    using TElement = Internal::TRangeElementValue<TRangeType>;

    if constexpr ( Internal::CPointerReturnRange<TRangeType> )
    {
        if ( auto *Result = FindIfNot( std::forward<TRangeType>( InRange ), std::forward<TPredicate>( InPredicate ) ) )
        {
            return TOptional<TElement>( *Result );
        }
    }
    else
    {
        auto Last   = End( InRange );
        auto Result = FindIfNot( Begin( InRange ), Last, std::forward<TPredicate>( InPredicate ) );
        if ( Result != Last )
        {
            if constexpr ( Internal::bConsumeRangeElements<TRangeType> )
            {
                return TOptional<TElement>( IterMove( Result ) );
            }
            else
            {
                return TOptional<TElement>( *Result );
            }
        }
    }
    return TOptional<TElement>();
}
