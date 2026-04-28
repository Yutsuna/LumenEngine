/**
 * @file EqualityComparable.hpp
 * @brief Concepts for Equality Comparable types in LumenEngine
 */

#pragma once

#include "Concepts/ConvertibleTo.hpp"

namespace LumenEngine
{

namespace Concepts
{
    /**
     * @concept CEqualityComparable
     * @brief Constrains types that can be compared for equality using operator==
     *
     * @tparam ObjectType The type to check for equality comparability
     *
     * @code
     *   ObjectType A, B;
     *   Bool bEqual = (A == B);
     * @endcode
     */
    template <typename ObjectType>
    concept CEqualityComparable = requires( const ObjectType &A, const ObjectType &B ) {
        { A == B } -> CConvertibleTo<ObjectType>;
    };

} // namespace Concepts

} // namespace LumenEngine
