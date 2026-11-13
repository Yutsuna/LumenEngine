/**
 * @file CommandLineTests.cpp
 * @brief Functional tests for FCommandLine parsing.
 */

#include <CLI/CommandLine.hpp>
#include <String/StringView.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Private
 */

namespace
{

    struct FCommandLineHolder
    {
        TVector<const AnsiChar *> Args;
        Launch::FCommandLine CommandLine;
    };

    [[nodiscard]] FCommandLineHolder MakeCommandLine ( const std::initializer_list<const AnsiChar *> InArgs ) noexcept
    {
        FCommandLineHolder Holder;
        Holder.Args        = TVector<const AnsiChar *>( InArgs );
        Holder.CommandLine = Launch::FCommandLine( static_cast<Int32>( Holder.Args.size() ), Holder.Args.data() );
        return Holder;
    }

} // namespace

/**
 * Public
 */

LUMEN_TEST( FCommandLine_Construct, EmptyIsValid )
{
    const Launch::FCommandLine CommandLine;

    LUMEN_CHECK_TRUE( CommandLine.GetProgramName().empty() );
    LUMEN_CHECK_TRUE( CommandLine.GetArguments().empty() );
}

LUMEN_TEST( FCommandLine_Construct, NullArgvIsTreatedAsEmpty )
{
    const Launch::FCommandLine CommandLine( 3, nullptr );

    LUMEN_CHECK_TRUE( CommandLine.GetProgramName().empty() );
    LUMEN_CHECK_TRUE( CommandLine.GetArguments().empty() );
}

LUMEN_TEST( FCommandLine_Construct, NegativeArgcIsTreatedAsEmpty )
{
    const AnsiChar *const Arguments[] = { "prog", "-a" };
    const Launch::FCommandLine CommandLine( -1, Arguments );

    LUMEN_CHECK_TRUE( CommandLine.GetProgramName().empty() );
    LUMEN_CHECK_TRUE( CommandLine.GetArguments().empty() );
}

LUMEN_TEST( FCommandLine_Construct, NullProgramNameIsTolerated )
{
    const AnsiChar *const Arguments[] = { nullptr, "-a" };
    const Launch::FCommandLine CommandLine( 2, Arguments );

    LUMEN_CHECK_TRUE( CommandLine.GetProgramName().empty() );
    LUMEN_CHECK_EQ( CommandLine.GetArguments().size(), 1ULL );
}

LUMEN_TEST( FCommandLine_GetProgramName, ReturnsArgvZero )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-headless" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;

    LUMEN_CHECK_TRUE( CommandLine.GetProgramName() == "Lumen" );
}

LUMEN_TEST( FCommandLine_GetArguments, SkipsProgramName )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-a", "-b=c" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;
    const auto Arguments                    = CommandLine.GetArguments();

    LUMEN_CHECK_EQ( Arguments.size(), 2ULL );
    LUMEN_CHECK_TRUE( FStringView( Arguments[0] ) == "-a" );
    LUMEN_CHECK_TRUE( FStringView( Arguments[1] ) == "-b=c" );
}

LUMEN_TEST( FCommandLine_HasSwitch, DetectsBareSwitch )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-headless" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;

    LUMEN_CHECK_TRUE( CommandLine.HasSwitch( "headless" ) );
}

LUMEN_TEST( FCommandLine_HasSwitch, IsCaseInsensitive )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-HeadLess" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;

    LUMEN_CHECK_TRUE( CommandLine.HasSwitch( "headless" ) );
    LUMEN_CHECK_TRUE( CommandLine.HasSwitch( "HEADLESS" ) );
}

LUMEN_TEST( FCommandLine_HasSwitch, DoesNotMatchKeyValuePairs )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-headless=true" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;

    LUMEN_CHECK_FALSE( CommandLine.HasSwitch( "headless" ) );
}

LUMEN_TEST( FCommandLine_HasSwitch, DoubleDashIsSupported )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "--headless" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;

    LUMEN_CHECK_TRUE( CommandLine.HasSwitch( "headless" ) );
}

LUMEN_TEST( FCommandLine_GetValue, ReturnsValueForKeyValuePair )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-fps=60" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;
    const auto Result                       = CommandLine.GetValue( "fps" );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( *Result == "60" );
}

LUMEN_TEST( FCommandLine_GetValue, IsCaseInsensitive )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-FPS=60" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;
    const auto Result                       = CommandLine.GetValue( "fps" );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( *Result == "60" );
}

LUMEN_TEST( FCommandLine_GetValue, SupportsDoubleDash )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "--fps=60" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;
    const auto Result                       = CommandLine.GetValue( "fps" );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( *Result == "60" );
}

LUMEN_TEST( FCommandLine_GetValue, ReturnsErrorWhenKeyMissing )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-fps=60" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;
    const auto Result                       = CommandLine.GetValue( "width" );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::MissingArgument );
}

LUMEN_TEST( FCommandLine_GetNumber, ParsesInteger )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-maxframes=42" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;
    const auto Result                       = CommandLine.GetNumber<UInt64>( "maxframes" );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( *Result, 42ULL );
}

LUMEN_TEST( FCommandLine_GetNumber, ParsesFloatingPoint )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-fixedstep=0.02" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;
    const auto Result                       = CommandLine.GetNumber<Float64>( "fixedstep" );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_TRUE( *Result == 0.02 );
}

LUMEN_TEST( FCommandLine_GetNumber, ReturnsErrorForMalformedValue )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-fixedstep=abc" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;
    const auto Result                       = CommandLine.GetNumber<Float64>( "fixedstep" );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::InvalidArgument );
}

LUMEN_TEST( FCommandLine_GetNumberOr, ReturnsValueWhenPresent )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-fps=30" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;
    const auto Result                       = CommandLine.GetNumberOr<Float64>( "fps", 60.0 );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( *Result, 30.0 );
}

LUMEN_TEST( FCommandLine_GetNumberOr, ReturnsFallbackWhenAbsent )
{
    const auto Holder                       = MakeCommandLine( { "Lumen" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;
    const auto Result                       = CommandLine.GetNumberOr<Float64>( "fps", 60.0 );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( *Result, 60.0 );
}

LUMEN_TEST( FCommandLine_GetNumberOr, ErrorsWhenSwitchHasNoValue )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-fps" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;
    const auto Result                       = CommandLine.GetNumberOr<Float64>( "fps", 60.0 );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::MissingArgument );
}

LUMEN_TEST( FCommandLine_ForEachOption, VisitsEveryArgument )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-a", "--b=c", "positional" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;

    UInt32 Count = 0U;
    CommandLine.ForEachOption( [&Count] ( const AnsiChar *const, const FStringView ) noexcept
    {
        ++Count;
    } );

    LUMEN_CHECK_EQ( Count, 3U );
}

LUMEN_TEST( FCommandLine_ForEachOption, StripsDashesAndValues )
{
    const auto Holder                       = MakeCommandLine( { "Lumen", "-a", "--b=c", "positional" } );
    const Launch::FCommandLine &CommandLine = Holder.CommandLine;

    TVector<FStringView> Names;
    CommandLine.ForEachOption( [&Names] ( const AnsiChar *const, const FStringView InName ) noexcept
    {
        Names.push_back( InName );
    } );

    LUMEN_CHECK_EQ( Names.size(), 3ULL );
    LUMEN_CHECK_TRUE( Names[0] == "a" );
    LUMEN_CHECK_TRUE( Names[1] == "b" );
    LUMEN_CHECK_TRUE( Names[2].empty() );
}

} // namespace Lumen
