/**
 * @file Expected.hpp
 * @brief Implementation of the Expected type.
 */

#pragma once

#include "Container/String.hpp"

#include <expected>

namespace LumenEngine
{

/**
 * @typedef TExpected
 * @brief Represents either a Value or an Error
 *
 * @tparam ValueType The type of the value.
 * @tparam ErrorType The type of the error.
 */
template <typename ValueType, typename ErrorType> using TExpected = std::expected<ValueType, ErrorType>;

/**
 * @brief Helper function to create an unexpected value.
 *
 * @tparam ErrorType The type of the error.
 * @param InError The error to encapsulate.
 *
 * @return An unexpected value containing the error.
 */
template <typename ErrorType> [[nodiscard]] inline auto MakeUnexpected ( ErrorType InError )
{
    return std::unexpected( std::move( InError ) );
}

/**
 * @macro LUMEN_EXPECT
 * @brief Evaluates a condition and returns an unexpected value if the condition is false.
 */
#define LUMEN_EXPECT( Condition, Error )                                                                                                                                 \
    do                                                                                                                                                                   \
    {                                                                                                                                                                    \
        if ( not( Condition ) )                                                                                                                                          \
        {                                                                                                                                                                \
            return MakeUnexpected( Error );                                                                                                                              \
        }                                                                                                                                                                \
    } while ( false )

/**
 * @macro LUMEN_EXPECT_VALUE
 * @brief Evaluates an expected value and returns an unexpected value if it is not a valid value.
 */
#define LUMEN_EXPECT_VALUE( ExpectedValue )                                                                                                                              \
    do                                                                                                                                                                   \
    {                                                                                                                                                                    \
        if ( not( ExpectedValue ).has_value() )                                                                                                                          \
        {                                                                                                                                                                \
            return MakeUnexpected( ( ExpectedValue ).error() );                                                                                                          \
        }                                                                                                                                                                \
    } while ( false )

} // namespace LumenEngine