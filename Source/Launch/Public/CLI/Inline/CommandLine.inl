/**
- @file CommandLine.inl
- @brief Template definitions of FCommandLine.
 */

#pragma once

#include <CLI/CommandLine.hpp>

#include <String/StringUtils.hpp>

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
    if ( HasSwitch( InKey ) ) [[unlikely]]
    {
        return LaunchFailure( ELaunchStage::PreInit, ELaunchErrorCode::MissingArgument,
                              "Numeric command-line option requires a value" );
    }

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

template <typename TFunction> void Launch::FCommandLine::ForEachOption ( TFunction &&InCallback ) const noexcept
{
    for ( const AnsiChar *const Raw : Arguments )
    {
        FStringView Option = AsOption( Raw );

        if ( const USize Eq = Option.find( '=' ); Eq != FStringView::npos )
        {
            Option = Option.substr( 0, Eq );
        }
        InCallback( Raw, Option );
    }
}

} // namespace Lumen
