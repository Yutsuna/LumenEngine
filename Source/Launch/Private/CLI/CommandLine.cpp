///
/// @file CommandLine.cpp
/// @brief FCommandLine implementation.
///

#include <CLI/CommandLine.hpp>
#include <String/StringUtils.hpp>

namespace Lumen
{
Launch::FCommandLine::FCommandLine ( const Int32 InArgc, const AnsiChar *const *InArgv ) noexcept
{
    if ( InArgv == nullptr or InArgc <= 0 )
    {
        return;
    }

    ProgramName = FStringView( InArgv[0] );
    Arguments   = TSpan<const AnsiChar *const>( InArgv + 1, static_cast<USize>( InArgc - 1 ) );
}

FStringView Launch::FCommandLine::GetProgramName () const noexcept
{
    return ProgramName;
}

Bool Launch::FCommandLine::HasSwitch ( const FStringView InName ) const noexcept
{
    for ( const AnsiChar *const Raw : Arguments )
    {
        if ( Raw != nullptr and EqualsIgnoreCase( StripDashes( FStringView( Raw ) ), InName ) )
        {
            return true;
        }
    }

    return false;
}

Launch::TLaunchResult<FStringView> Launch::FCommandLine::GetValue ( const FStringView InKey ) const noexcept
{
    for ( const AnsiChar *const Raw : Arguments )
    {
        if ( Raw == nullptr )
        {
            continue;
        }

        const FStringView Option = StripDashes( FStringView( Raw ) );

        if ( const USize SeparatorIndex = Option.find( '=' );
             SeparatorIndex != FStringView::npos and EqualsIgnoreCase( Option.substr( 0, SeparatorIndex ), InKey ) )
        {
            return Option.substr( SeparatorIndex + 1 );
        }
    }

    return LaunchFailure( ELaunchStage::PreInit, ELaunchErrorCode::MissingArgument, "Command-line key not found" );
}
} // namespace Lumen
