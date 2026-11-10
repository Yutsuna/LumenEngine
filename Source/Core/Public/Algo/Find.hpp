/**
 * @file Find.hpp
 * @brief Find algorithms over ranges.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <Algo/RangeTraits.hpp>

#include <Templates/Optional.hpp>

namespace Lumen
{

namespace Algo
{

    /**
     * Iterator
     */

    template <class TIterator, class TPredicate>
    [[nodiscard]] constexpr TIterator FindIf ( TIterator InFirst, TIterator InLast, TPredicate &&InPredicate );

    template <class TIterator, class TValue>
    [[nodiscard]] constexpr TIterator Find ( TIterator InFirst, TIterator InLast, const TValue &InValue );

    template <class TIterator, class TPredicate>
    [[nodiscard]] constexpr TIterator FindIfNot ( TIterator InFirst, TIterator InLast, TPredicate &&InPredicate );

    template <class TIterator, class TValue, class TComparator>
    [[nodiscard]] constexpr TIterator
    FindIfEqual ( TIterator InFirst, TIterator InLast, const TValue &InValue, TComparator &&InComparator );

    /**
     * TOptional<TIterator>
     */

    template <class TIterator, class TPredicate>
    [[nodiscard]] constexpr TOptional<TIterator> FindIfOrNone ( TIterator InFirst, TIterator InLast, TPredicate &&InPredicate );

    template <class TIterator, class TValue>
    [[nodiscard]] constexpr TOptional<TIterator> FindOrNone ( TIterator InFirst, TIterator InLast, const TValue &InValue );

    template <class TIterator, class TPredicate>
    [[nodiscard]] constexpr TOptional<TIterator>
    FindIfNotOrNone ( TIterator InFirst, TIterator InLast, TPredicate &&InPredicate );

    template <class TIterator, class TValue, class TComparator>
    [[nodiscard]] constexpr TOptional<TIterator>
    FindIfEqualOrNone ( TIterator InFirst, TIterator InLast, const TValue &InValue, TComparator &&InComparator );

    /**
     * TPtr
     */

    template <class TRangeType, class TPredicate>
    [[nodiscard]] constexpr Internal::TRangeElementPtr<TRangeType> FindIf ( TRangeType &&InRange, TPredicate &&InPredicate );

    template <class TRangeType, class TValue>
    [[nodiscard]] constexpr Internal::TRangeElementPtr<TRangeType> Find ( TRangeType &&InRange, const TValue &InValue );

    template <class TRangeType, class TPredicate>
    [[nodiscard]] constexpr Internal::TRangeElementPtr<TRangeType> FindIfNot ( TRangeType &&InRange, TPredicate &&InPredicate );

    /**
     * TOptional<Value>
     */

    template <class TRangeType, class TPredicate>
    [[nodiscard]] constexpr TOptional<Internal::TRangeElementValue<TRangeType>> FindIfOrNone ( TRangeType &&InRange,
                                                                                               TPredicate &&InPredicate );

    template <class TRangeType, class TValue>
    [[nodiscard]] constexpr TOptional<Internal::TRangeElementValue<TRangeType>> FindOrNone ( TRangeType &&InRange,
                                                                                             const TValue &InValue );

    template <class TRangeType, class TPredicate>
    [[nodiscard]] constexpr TOptional<Internal::TRangeElementValue<TRangeType>> FindIfNotOrNone ( TRangeType &&InRange,
                                                                                                  TPredicate &&InPredicate );

} // namespace Algo

} // namespace Lumen

#include "Inline/Find.inl"
