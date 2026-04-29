/**
 * @file Optional.hpp
 * @brief Optional type
 */

#include <optional>

#pragma once

namespace LumenEngine
{

template <typename Type> using TOptional = std::optional<Type>;

#define LUMEN_OPTIONAL( OptionalValue )                                                                                                                                  \
    do                                                                                                                                                                   \
    {                                                                                                                                                                    \
        if ( not( OptionalValue ).has_value() )                                                                                                                          \
        {                                                                                                                                                                \
            return std::nullopt;                                                                                                                                         \
        }                                                                                                                                                                \
    } while ( false )

} // namespace LumenEngine
