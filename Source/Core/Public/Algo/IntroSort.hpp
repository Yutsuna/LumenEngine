/**
 * @file IntroSort.hpp
 * @brief Introspective sort declaration: quicksort with a headsort
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

#include <Concepts/Sortable.hpp>

#include <Algo/QuickSort.hpp>

namespace Lumen
{

namespace Algo
{

    namespace Internal
    {

        /**
         * @brief Introspective sort loop implementation.
         *
         * @param InFirst Iterator to the first element of the sequence.
         * @param InLast Iterator to the last element of the sequence.
         * @param InDepthLimit Maximum depth limit for the introspective sort.
         * @param InPredicate Predicate used to compare elements.
         */
        template <typename TIterator, typename TPredicate>
        void IntroSortLoop ( TIterator InFirst, TIterator InLast, Int32 InDepthLimit, TPredicate &InPredicate ) noexcept;

    } // namespace Internal

    /**
     * @brief Introspective sort implementation.
     *
     * @param InRange Range to sort.
     */
    template <typename TRangeType>
        requires Concepts::CSortableRange<TRangeType, std::less<>>
    void IntroSort ( TRangeType &&InRange ) noexcept;

    /**
     * @brief Introspective sort implementation with a custom predicate.
     *
     * @param InRange Range to sort.
     * @param InPredicate Predicate used to compare elements.
     */
    template <typename TRangeType, typename TPredicate>
        requires Concepts::CSortableRange<TRangeType, TPredicate>
    void IntroSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept;

} // namespace Algo

} // namespace Lumen

#include "Inline/IntroSort.inl"
