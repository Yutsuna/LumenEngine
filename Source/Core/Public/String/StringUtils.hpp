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
    return InChar == ' ' or ( static_cast<UInt8>( InChar ) - static_cast<UInt8>( '\t' ) < 5u );
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
        if ( not IsSpace( *Ptr ) ) [[likely]]
        {
            return false;
        }
    }
    return true;
}

/// @brief Checks if the given string view contains only space characters.
[[nodiscard]] constexpr CORE_EXPORT Bool IsSpace ( FStringView InStringView ) noexcept
{
    return std::ranges::all_of( InStringView, [] ( const AnsiChar c ) -> Bool { return IsSpace( c ); } );
}

/// @brief Checks if the given string contains only space characters.
[[nodiscard]] constexpr CORE_EXPORT Bool IsSpace ( const FString &InString ) noexcept
{
    return IsSpace( FStringView( InString ) );
}

} // namespace Lumen