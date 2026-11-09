/**
 * @file Sort.inl
 * @brief Inline implementation of the Sort algorithm.
 */

#pragma once

#include <Algo/Sort.hpp>
#include <Algo/IntroSort.hpp>
#include <Algo/MergeSort.hpp>

#include <functional>
#include <utility>

/**
 * Public
 */

template <typename TRangeType>
    requires Lumen::Concepts::CSortableRange<TRangeType, std::less<>>
void Lumen::Algo::Sort ( TRangeType &&InRange ) noexcept
{
    IntroSort( std::forward<TRangeType>( InRange ) );
}

template <typename TRangeType, typename TPredicate>
    requires Lumen::Concepts::CSortableRange<TRangeType, TPredicate>
void Lumen::Algo::Sort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept
{
    IntroSort( std::forward<TRangeType>( InRange ), std::move( InPredicate ) );
}

template <typename TRangeType>
    requires Lumen::Concepts::CSortableRange<TRangeType, std::less<>>
void Lumen::Algo::StableSort ( TRangeType &&InRange ) noexcept
{
    MergeSort( std::forward<TRangeType>( InRange ) );
}

template <typename TRangeType, typename TPredicate>
    requires Lumen::Concepts::CSortableRange<TRangeType, TPredicate>
void Lumen::Algo::StableSort ( TRangeType &&InRange, TPredicate InPredicate ) noexcept
{
    MergeSort( std::forward<TRangeType>( InRange ), std::move( InPredicate ) );
}
