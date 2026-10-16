///
/// @file CommandLine.inl
/// @brief Template definitions of FCommandLine.
///

#pragma once

#include <CLI/CommandLine.hpp>

#include <charconv>
#include <system_error>

namespace Lumen
{

template <Launch::Concepts::CCommandLineNumber NumberType>
Launch::TLaunchResult<NumberType> Launch::FCommandLine::GetNumber ( const FStringView InKey ) const noexcept
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

template <Launch::Concepts::CCommandLineNumber NumberType>
Launch::TLaunchResult<NumberType> Launch::FCommandLine::GetNumberOr ( const FStringView InKey,
                                                                      const NumberType InFallback ) const noexcept
{
    return GetNumber<NumberType>( InKey ).OrElse(
        [InFallback] ( const FLaunchError &InError ) noexcept -> TLaunchResult<NumberType>
        {
            if ( InError.Code == ELaunchErrorCode::MissingArgument )
            {
                return InFallback;
            }

            return MakeUnexpected( InError );
        } );
}

} // namespace Lumen
