/**
 * @file RangeTraits.hpp
 * @brief Range introspection traits shared by the Algo module.
 */

#pragma once

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
        using TRangeElementPtr = decltype( [] ( auto &&Range )
        {
            for ( auto &&Element : Range )
            {
                return &Element;
            }
        }( std::declval<TRangeType>() ) );

        /**
         * @brief Value type of the elements of TRangeType (cv/ref/pointer stripped).
         */
        template <class TRangeType>
        using TRangeElementValue = std::remove_cvref_t<std::remove_pointer_t<TRangeElementPtr<TRangeType>>>;

        /**
         * @brief Reference value type of the elements of TRangeType (cv/ref/pointer stripped).
         */
        template <class TRangeType> using TRangeReferenceT = std::remove_pointer_t<TRangeElementPtr<TRangeType>> &;

    } // namespace Internal

} // namespace Algo

} // namespace Lumen
