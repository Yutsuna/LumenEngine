/**
 * @file UnitTestsAssert.hpp
 * @brief Assertion functions and macros for Lumen unit tests.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>

#include <String/String.hpp>
#include <String/StringView.hpp>
#include <Tests/UnitTestAssertionFailure.hpp>
#include <Tests/UnitTestTypes.hpp>

#include <Concepts/IsIntegral.hpp>

#include <format>
#include <source_location>
#include <type_traits>
#include <utility>

namespace Lumen
{

namespace Tests
{

    template <typename T>
    concept CFormattableOrEnum = std::formattable<std::remove_cvref_t<T>, AnsiChar> or std::is_enum_v<std::remove_cvref_t<T>>;

    template <typename T> constexpr decltype( auto ) ToPrintable ( const T &Value ) noexcept
    {
        if constexpr ( std::is_enum_v<std::remove_cvref_t<T>> )
        {
            return std::to_underlying( Value );
        }
        else if constexpr ( std::formattable<std::remove_cvref_t<T>, AnsiChar> )
        {
            return Value;
        }
        else
        {
            return "<unformattable>";
        }
    }

    /**
     * @brief Compares two values for equality, using a sign-safe comparison when both are integers.
     * @details std::cmp_equal avoids the signed/unsigned promotion pitfalls of operator== and is -Wsign-compare clean,
     *          which matters when comparing C flag typedefs (uint32_t) with enum bit constants (int), e.g. Vulkan's.
     */
    template <typename TLeft, typename TRight>
    [[nodiscard]] constexpr Bool AreEqual ( const TLeft &Left, const TRight &Right )
    {
        if constexpr ( Concepts::CIsIntegral<std::remove_cvref_t<TLeft>> and Concepts::CIsIntegral<std::remove_cvref_t<TRight>> )
        {
            return std::cmp_equal( Left, Right );
        }
        else
        {
            return Left == Right;
        }
    }

    /**
     * @brief Asserts that two expressions are equal.
     */
    template <typename TLeft, typename TRight>
    void AssertEqual ( const TLeft &Left,
                       const TRight &Right,
                       const FStringView LeftExpression,
                       const FStringView RightExpression,
                       const std::source_location Location = std::source_location::current() )
    {
        if ( not AreEqual( Left, Right ) ) [[unlikely]]
        {
            if constexpr ( CFormattableOrEnum<TLeft> and CFormattableOrEnum<TRight> )
            {
                throw FUnitTestAssertionFailure( std::format( "Check equal assertion failed: ({}) == ({}) [{} != {}]",
                                                              LeftExpression, RightExpression, ToPrintable( Left ),
                                                              ToPrintable( Right ) ),
                                                 Location );
            }
            else
            {
                throw FUnitTestAssertionFailure(
                    std::format( "Check equal assertion failed: ({}) == ({})", LeftExpression, RightExpression ), Location );
            }
        }
    }

    /**
     * @brief Asserts that two expressions are not equal.
     */
    template <typename TLeft, typename TRight>
    void AssertNotEqual ( const TLeft &Left,
                          const TRight &Right,
                          const FStringView LeftExpression,
                          const FStringView RightExpression,
                          const std::source_location Location = std::source_location::current() )
    {
        if ( AreEqual( Left, Right ) ) [[unlikely]]
        {
            if constexpr ( CFormattableOrEnum<TLeft> and CFormattableOrEnum<TRight> )
            {
                throw FUnitTestAssertionFailure( std::format( "Check not-equal assertion failed: ({}) != ({}) [both are {}]",
                                                              LeftExpression, RightExpression, ToPrintable( Left ) ),
                                                 Location );
            }
            else
            {
                throw FUnitTestAssertionFailure(
                    std::format( "Check not-equal assertion failed: ({}) != ({})", LeftExpression, RightExpression ), Location );
            }
        }
    }

} // namespace Tests

} // namespace Lumen

#define LUMEN_CHECK( Expression )                                                                                                \
    do                                                                                                                           \
    {                                                                                                                            \
        if ( not static_cast<::Lumen::Bool>( Expression ) ) [[unlikely]]                                                         \
        {                                                                                                                        \
            throw ::Lumen::Tests::FUnitTestAssertionFailure( ::std::format( "Check assertion failed: ({})", #Expression ),       \
                                                             ::std::source_location::current() );                                \
        }                                                                                                                        \
    } while ( false )

#define LUMEN_CHECK_MSG( Expression, Message )                                                                                   \
    do                                                                                                                           \
    {                                                                                                                            \
        if ( not static_cast<::Lumen::Bool>( Expression ) ) [[unlikely]]                                                         \
        {                                                                                                                        \
            throw ::Lumen::Tests::FUnitTestAssertionFailure(                                                                     \
                ::std::format( "Check assertion failed: ({}) -> {}", #Expression, Message ),                                     \
                ::std::source_location::current() );                                                                             \
        }                                                                                                                        \
    } while ( false )

#define LUMEN_CHECK_EQ( LeftExpression, RightExpression )                                                                        \
    ::Lumen::Tests::AssertEqual( ( LeftExpression ), ( RightExpression ), #LeftExpression, #RightExpression,                     \
                                 ::std::source_location::current() )

#define LUMEN_CHECK_NOT_EQ( LeftExpression, RightExpression )                                                                    \
    ::Lumen::Tests::AssertNotEqual( ( LeftExpression ), ( RightExpression ), #LeftExpression, #RightExpression,                  \
                                    ::std::source_location::current() )

#define LUMEN_CHECK_TRUE( Expression ) LUMEN_CHECK( ( Expression ) == true )
#define LUMEN_CHECK_FALSE( Expression ) LUMEN_CHECK( ( Expression ) == false )

#define LUMEN_CHECK_HAS_VALUE( ExpectedOrOptional )                                                                              \
    do                                                                                                                           \
    {                                                                                                                            \
        if ( not( ExpectedOrOptional ).HasValue() ) [[unlikely]]                                                                 \
        {                                                                                                                        \
            throw ::Lumen::Tests::FUnitTestAssertionFailure(                                                                     \
                ::std::format( "Target container expected to contain value, but was unvalued: ({})", #ExpectedOrOptional ),      \
                ::std::source_location::current() );                                                                             \
        }                                                                                                                        \
    } while ( false )

#define LUMEN_CHECK_HAS_ERROR( ExpectedObject )                                                                                  \
    do                                                                                                                           \
    {                                                                                                                            \
        if ( ( ExpectedObject ).HasValue() ) [[unlikely]]                                                                        \
        {                                                                                                                        \
            throw ::Lumen::Tests::FUnitTestAssertionFailure(                                                                     \
                ::std::format( "Target expected to contain error, but holds an unexpected valid value: ({})", #ExpectedObject ), \
                ::std::source_location::current() );                                                                             \
        }                                                                                                                        \
    } while ( false )
