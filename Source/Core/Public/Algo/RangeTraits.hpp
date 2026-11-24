/**
 * @file RangeTraits.hpp
 * @brief Range introspection traits shared by the Algo module.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace Lumen
{

namespace Algo
{

    namespace Internal
    {

        /**
         * @brief Yields the pointer type produced by range-for over TRangeType.
         */
        template <class TRangeType>
        using TRangeElementPtr = std::remove_reference_t<decltype( *std::begin( std::declval<TRangeType>() ) )> *;

        /**
         * @brief Value type of the elements of TRangeType (cv/ref/pointer stripped).
         */
        template <class TRangeType>
        using TRangeElementValue = std::remove_cvref_t<std::remove_pointer_t<TRangeElementPtr<TRangeType>>>;

        /**
         * @brief Reference value type of the elements of TRangeType (cv/ref/pointer stripped).
         */
        template <class TRangeType> using TRangeReferenceT = std::remove_pointer_t<TRangeElementPtr<TRangeType>> &;

        template <class TRangeType>
        concept CPointerReturnRange = std::ranges::range<TRangeType> && std::ranges::borrowed_range<TRangeType> &&
                                      std::is_reference_v<std::ranges::range_reference_t<TRangeType>>;

        template <class TRangeType>
        inline constexpr Bool bConsumeRangeElements =
            not std::is_lvalue_reference_v<TRangeType> && not std::ranges::borrowed_range<TRangeType>;

    } // namespace Internal

} // namespace Algo

} // namespace Lumen
