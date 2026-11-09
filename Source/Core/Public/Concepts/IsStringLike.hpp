/**
 * @file IsStringLike.hpp
 * @brief Concept to check if a type is a string-like type.
 */

#pragma once

#include <Concepts/ConvertibleTo.hpp>
#include <String/String.hpp>

namespace Lumen
{

namespace Concepts
{

    /**
     * @concepts CIsStringLike
     * @brief Concept to check if a type is a string-like type.
     */
    template <class Type>
    concept CIsStringLike = CConvertibleTo<Type, FString>;
} // namespace Concepts

} // namespace Lumen
