/**
 * @file Optional.hpp
 * @brief Optional type
 */

#include <optional>

#pragma once

namespace LumenEngine
{

template <typename Type> using TOptional = std::optional<Type>;

/**
 * @macro LUMEN_OPTIONAL_ERROR
 * @brief Checks if the optional value has a value, and if not, returns the specified error.
 */
#define LUMEN_OPTIONAL_ERROR( OptionalValue, Error )                                                                                                                     \
    do                                                                                                                                                                   \
    {                                                                                                                                                                    \
        if ( not( OptionalValue ).has_value() )                                                                                                                          \
        {                                                                                                                                                                \
            return Error;                                                                                                                                                \
        }                                                                                                                                                                \
    } while ( false )

/**
 * @macro LUMEN_OPTIONAL
 * @brief Checks if the optional value has a value, and if not, returns std::nullopt.
 */
#define LUMEN_OPTIONAL( OptionalValue ) LUMEN_OPTIONAL_ERROR( OptionalValue, std::nullopt )

} // namespace LumenEngine
