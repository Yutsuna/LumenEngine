/**
 * @file Equal.hpp
 * @brief Equality comparison algorithms over ranges.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

#include <Algo/RangeTraits.hpp>

#include <Concepts/ConvertibleTo.hpp>
#include <Templates/Invoke.hpp>

namespace Lumen
{

namespace Algo
{

    /**
     * @brief Returns true if the two ranges contain the same elements in the same order,
     *        compared with the given binary predicate.
     *
     * @param InRangeA The first range to compare.
     * @param InRangeB The second range to compare.
     * @param InPredicate Binary predicate returning true when its two arguments compare equal.
     * @return True if both ranges have the same length and every pair of elements compares equal.
     */
    template <typename TRangeA, typename TRangeB, typename TPredicate>
        requires( Concepts::CConvertibleTo<
                  TInvokeResultT<TPredicate, Internal::TRangeReferenceT<TRangeA>, Internal::TRangeReferenceT<TRangeB>>,
                  Bool> )
    [[nodiscard]] LUMEN_FORCE_INLINE constexpr Bool Equal ( TRangeA &&InRangeA, TRangeB &&InRangeB, TPredicate &&InPredicate );

    /**
     * @brief Returns true if the two ranges contain equal elements in the same order (`operator==`).
     *
     * @param InRangeA The first range to compare.
     * @param InRangeB The second range to compare.
     */
    template <typename TRangeA, typename TRangeB>
    [[nodiscard]] LUMEN_FORCE_INLINE constexpr Bool Equal ( TRangeA &&InRangeA, TRangeB &&InRangeB );

} // namespace Algo

} // namespace Lumen

#include "Inline/Equal.inl"
