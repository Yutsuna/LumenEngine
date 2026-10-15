/// @file StringUtils.hpp
/// @brief String utilities functions.

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>

#include <String/String.hpp>
#include <String/StringView.hpp>

#include <algorithm>

namespace Lumen
{
/// @brief Checks if the given character is a space character.
[[nodiscard]] constexpr CORE_EXPORT Bool IsSpace ( const AnsiChar InChar ) noexcept
{
    return InChar == ' ' or static_cast<UInt32>( InChar - '\t' ) < 5U;
}

/// @brief Checks if the given null-terminated string contains only space characters.
[[nodiscard]] constexpr CORE_EXPORT Bool IsSpace ( const AnsiChar *const InCString ) noexcept
{
    if ( InCString == nullptr ) [[unlikely]]
    {
        return false;
    }

    for ( const AnsiChar *Ptr = InCString; *Ptr != '\0'; ++Ptr )
    {
        if ( not IsSpace( *Ptr ) )
        {
            return false;
        }
    }
    return true;
}

/// @brief Checks if the given string view contains only space characters.
[[nodiscard]] constexpr CORE_EXPORT Bool IsSpace ( FStringView InStringView ) noexcept
{
    return std::ranges::all_of( InStringView, [] ( const AnsiChar c ) noexcept { return IsSpace( c ); } );
}

/// @brief To lower ASCII (Branchless-friendly single comparison)
[[nodiscard]] constexpr CORE_EXPORT AnsiChar ToLower ( const AnsiChar InChar ) noexcept
{
    const auto U = static_cast<UInt8>( InChar );
    return ( static_cast<UInt8>( U - 'A' ) < 26u ) ? static_cast<AnsiChar>( U | 0x20 ) : InChar;
}

/// @brief Converts a StringView to a lower-case FString (Allocates).
/// Note: Handles both FString and FStringView via implicit conversion.
[[nodiscard]] constexpr CORE_EXPORT FString ToLower ( FStringView InString )
{
    FString Result;
    Result.resize( InString.length() );

    std::ranges::transform( InString, Result.begin(), [] ( const AnsiChar c ) noexcept { return ToLower( c ); } );

    return Result;
}

/// @brief In-place conversion to lower-case (Zero allocations).
constexpr CORE_EXPORT void ToLowerInline ( FString &InOutString ) noexcept
{
    for ( AnsiChar &c : InOutString )
    {
        c = ToLower( c );
    }
}

/// @brief Case-insensitive comparison (Zero allocations, early exit).
[[nodiscard]] constexpr CORE_EXPORT Bool EqualsIgnoreCase ( FStringView InStringA, FStringView InStringB ) noexcept
{
    if ( InStringA.length() != InStringB.length() )
    {
        return false;
    }

    return std::ranges::equal( InStringA, InStringB,
                               [] ( const AnsiChar A, const AnsiChar B ) noexcept { return ToLower( A ) == ToLower( B ); } );
}

/// @brief Strips leading spaces
[[nodiscard]] constexpr CORE_EXPORT FStringView StripParam ( FStringView InArgument ) noexcept
{
    while ( not InArgument.empty() and InArgument.front() == ' ' )
    {
        InArgument.remove_prefix( 1 );
    }
    return InArgument;
}

/// @brief Strips dashes ( - and -- )
[[nodiscard]] constexpr CORE_EXPORT FStringView StripDashes ( FStringView InArgument ) noexcept
{
    if ( InArgument.starts_with( "--" ) )
    {
        InArgument.remove_prefix( 2 );
        return InArgument;
    }

    if ( InArgument.starts_with( '-' ) )
    {
        InArgument.remove_prefix( 1 );
        return InArgument;
    }

    return {};
}
} // namespace Lumen
