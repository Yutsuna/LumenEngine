///
/// @file CommandLine.cpp
/// @brief FCommandLine implementation.
///

#include <CLI/CommandLine.hpp>
#include <String/StringUtils.hpp>

namespace Lumen
{

namespace
{
    [[nodiscard]] FStringView AsOption ( const AnsiChar *const InRawCString ) noexcept
    {
        if ( InRawCString == nullptr or InRawCString[0] != '-' )
        {
            return {};
        }
        return StripDashes( FStringView( InRawCString ) );
    }
} // namespace

Launch::FCommandLine::FCommandLine ( const Int32 InArgc, const AnsiChar *const *InArgv ) noexcept
{
    if ( InArgv == nullptr or InArgc <= 0 )
    {
        return;
    }

    if ( InArgv[0] != nullptr )
    {
        ProgramName = FStringView( InArgv[0] );
    }

    Arguments = TSpan<const AnsiChar *const>( InArgv + 1, static_cast<USize>( InArgc - 1 ) );
}

FStringView Launch::FCommandLine::GetProgramName () const noexcept
{
    return ProgramName;
}

Bool Launch::FCommandLine::HasSwitch ( const FStringView InName ) const noexcept
{
    return std::ranges::any_of( Arguments,
                                [InName] ( const AnsiChar *const Raw ) noexcept -> Bool
                                {
                                    if ( const FStringView Option = AsOption( Raw );
                                         not Option.empty() and EqualsIgnoreCase( Option, InName ) )
                                    {
                                        return true;
                                    }
                                    return false;
                                } );
}

Launch::TLaunchResult<std::string_view> Launch::FCommandLine::GetValue ( const FStringView InKey ) const noexcept
{
    for ( const AnsiChar *const Raw : Arguments )
    {
        const FStringView Option = AsOption( Raw );

        if ( const USize SeparatorIndex = Option.find( '=' );
             SeparatorIndex != FStringView::npos and EqualsIgnoreCase( Option.substr( 0, SeparatorIndex ), InKey ) )
        {
            return Option.substr( SeparatorIndex + 1 );
        }
    }

    return LaunchFailure( ELaunchStage::PreInit, ELaunchErrorCode::MissingArgument, "Command-line key not found" );
}

} // namespace Lumen
