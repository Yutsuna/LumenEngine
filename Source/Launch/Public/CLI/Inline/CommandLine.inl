/// @file CommandLine.inl
/// @brief Inline declaration of FCommandLine

#pragma once

#include <CLI/CommandLine.hpp>

#include <charconv>

namespace Lumen
{
template <typename NumberType>
    requires( std::is_arithmetic_v<NumberType> and not std::is_same_v<NumberType, Bool> )
[[nodiscard]] Launch::TLaunchResult<NumberType> Launch::FCommandLine::GetNumber ( const FStringView InKey ) const noexcept
{
    return GetValue( InKey ).AndThen(
        [] ( const FStringView InText ) noexcept -> TLaunchResult<NumberType>
        {
            NumberType Parsed{};
            const AnsiChar *const Begin = InText.data();
            const AnsiChar *const End   = Begin + InText.size();

            const auto [Last, ErrorCode] = std::from_chars( Begin, End, Parsed );
            if ( ErrorCode != std::errc{} or Last != End ) [[unlikely]]
            {
                return LaunchFailure( ELaunchStage::PreInit, ELaunchErrorCode::InvalidArgument,
                                      "Malformed numeric command-line value" );
            }

            return Parsed;
        } );
}
} // namespace Lumen
