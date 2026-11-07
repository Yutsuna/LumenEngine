/**
 * @file LaunchEngineConfigParser.cpp
 * @brief Parse the Command Line then Validate the configuration.
 */

#include "Launch/LaunchError.hpp"
#include <Engine/LaunchEngineConfig.hpp>
#include <Engine/LaunchEngineConfigParser.hpp>

#include <Meta/Overloaded.hpp>
#include <Meta/Reflection.hpp>

#include <Logging/Logger.hpp>
#include <Templates/Array.hpp>

#include <cmath>
#include <string_view>
#include <type_traits>

static_assert( LUMEN_HAS_REFLECTION == 1, "LaunchEngineConfigParser requires C++26 static reflection." );

namespace Lumen
{

namespace
{

    LUMEN_LOG_DEFINE_CATEGORY( LogEngineConfigParser, "EngineConfigParser" );

    /** @brief Prevents `-fixedstep=1e-9` from freezing the engine in the catch-up loop. */
    constexpr Float64 MaxFixedStepsPerFrame = 1024.0;

    /** @brief Inline buffer for a lowercased option key. */
    struct FOptionKey
    {
        TArray<AnsiChar, 32> Buffer{};
        USize Length = 0;

        [[nodiscard]] FStringView View () const noexcept
        {
            return FStringView{ Buffer.data(), Length };
        }
    };

    /** @brief "MaxFrameTime" -> "maxframetime", "bHeadless" -> "headless". */
    [[nodiscard]] FOptionKey ToOptionKey ( const std::string_view InName, const Bool bIsBool ) noexcept
    {
        FOptionKey Key;

        const USize Begin   = ( bIsBool and not InName.empty() and InName.front() == 'b' ) ? 1 : 0;
        const USize Count   = std::min( InName.size() - Begin, Key.Buffer.size() );
        const AnsiChar *Src = InName.data() + Begin;
        AnsiChar *Dest      = Key.Buffer.data();

        for ( USize Index = 0; Index < Count; ++Index )
        {
            Dest[Index] = Lumen::ToLower( Src[Index] );
        }
        Key.Length = Count;

        return Key;
    }

    /** @brief Meta::ForEachMember, but the callback receives the CLI key instead of the raw member name. */
    template <Meta::CReflected TObject, typename TFunc>
    void ForEachConfigOption ( TObject &InObject, TFunc &&InCallback ) noexcept
    {
        Meta::ForEachMember( InObject, [&InCallback] ( const std::string_view InName, auto &InField ) noexcept
        {
            using FValue = std::remove_cvref_t<decltype( InField )>;
            InCallback( ToOptionKey( InName, Concepts::CSameAs<FValue, Bool> ), InField );
        } );
    }

    [[nodiscard]] Bool IsKnownOption ( const FStringView InName ) noexcept
    {
        if ( EqualsIgnoreCase( InName, "help" ) )
        {
            return true;
        }

        Bool bKnown = false;
        const Launch::FLaunchEngineConfig Defaults{};
        ForEachConfigOption( Defaults, [&bKnown, InName] ( const FOptionKey &InKey, const auto & ) noexcept
        {
            bKnown = bKnown or EqualsIgnoreCase( InName, InKey.View() );
        } );
        return bKnown;
    }

    [[nodiscard]] Launch::TLaunchResult<void> ValidateOptions ( const Launch::FCommandLine &InCommandLine ) noexcept
    {
        Launch::TLaunchResult<void> Result{};
        InCommandLine.ForEachOption( [&Result] ( const AnsiChar *const Raw, const FStringView Option ) noexcept
        {
            if ( not Result )
            {
                return;
            }
            if ( Option.empty() )
            {
                LUMEN_LOG_WARNING( LogEngineConfigParser, "Ignoring positional argument '{}'", Raw );
                return;
            }
            if ( not IsKnownOption( Option ) )
            {
                LUMEN_LOG_ERROR( LogEngineConfigParser, "Unknown command-line option '{}'", Option );
                Result = LaunchFailure( Launch::ELaunchStage::PreInit, Launch::ELaunchErrorCode::InvalidArgument,
                                        "Unknown command-line option" );
            }
        } );
        return Result;
    }

    [[nodiscard]] constexpr TUnexpected<Launch::FLaunchError> InvalidConfig ( const FStringView InMessage ) noexcept
    {
        return Launch::LaunchFailure( Launch::ELaunchStage::PreInit, Launch::ELaunchErrorCode::InvalidConfiguration, InMessage );
    }

    /** @brief Bind each member Bool -> -switch, arithmetic -> -key=value. */
    [[nodiscard]] Launch::TLaunchResult<Launch::FLaunchEngineConfig>
    BindOptions ( const Launch::FCommandLine &InCommandLine, const Launch::FLaunchEngineConfig &InDefaults ) noexcept
    {
        Launch::FLaunchEngineConfig Config = InDefaults;
        Launch::TLaunchResult<void> Result{};

        ForEachConfigOption( Config, [&InCommandLine, &Result] ( const FOptionKey &InKey, auto &InField ) noexcept
        {
            if ( not Result )
            {
                return; ///<< First error -> stop
            }

            const Meta::FOverloaded Bind{
                [&] ( Bool &InSwitch ) noexcept
            {
                InSwitch = InSwitch or InCommandLine.HasSwitch( InKey.View() );
            },
                [&]<Launch::Concepts::CCommandLineNumber TNumber>( TNumber &InValue ) noexcept
            {
                const Launch::TLaunchResult<TNumber> Parsed = InCommandLine.GetNumberOr( InKey.View(), InValue );
                if ( not Parsed ) [[unlikely]]
                {
                    Result = MakeUnexpected( Parsed.Error() );
                    return;
                }
                InValue = *Parsed;
            },
            };

            Bind( InField );
        } );

        if ( not Result ) [[unlikely]]
        {
            return MakeUnexpected( Result.Error() );
        }
        return Config;
    }

    void PrintUsage ( const Launch::FCommandLine &InCommandLine ) noexcept
    {
        LUMEN_LOG_DISPLAY( LogEngineConfigParser, "Usage: {} [-help]", InCommandLine.GetProgramName() );

        const Launch::FLaunchEngineConfig Defaults{};
        ForEachConfigOption( Defaults, [] ( const FOptionKey &InKey, const auto &InField ) noexcept
        {
            using FValue = std::remove_cvref_t<decltype( InField )>;

            if constexpr ( Concepts::CSameAs<FValue, Bool> )
            {
                LUMEN_LOG_DISPLAY( LogEngineConfigParser, "  -{}", InKey.View() );
            }
            else
            {
                LUMEN_LOG_DISPLAY( LogEngineConfigParser, "  -{}=<{}>", InKey.View(), Meta::TypeName<FValue>() );
            }
        } );
    }

} // namespace

Launch::TLaunchResult<Launch::FLaunchEngineConfig> Launch::ValidateEngineConfig ( const FLaunchEngineConfig &InConfig ) noexcept
{
    if ( not std::isfinite( InConfig.FixedStep ) or InConfig.FixedStep <= 0.0 ) [[unlikely]]
    {
        return InvalidConfig( "-fixedstep must be a finite value greater than 0" );
    }

    if ( not std::isfinite( InConfig.MaxFrameTime ) or InConfig.MaxFrameTime < InConfig.FixedStep ) [[unlikely]]
    {
        return InvalidConfig( "-maxframetime must be finite and not lower than -fixedstep" );
    }

    if ( InConfig.MaxFrameTime > InConfig.FixedStep * MaxFixedStepsPerFrame ) [[unlikely]]
    {
        return InvalidConfig( "-maxframetime / -fixedstep exceeds the allowed number of fixed steps per frame" );
    }

    if ( not std::isfinite( InConfig.MaxFPS ) or InConfig.MaxFPS < 0.0 ) [[unlikely]]
    {
        return InvalidConfig( "-maxfps must be a finite value greater than or equal to 0" );
    }

    return InConfig;
}

Launch::TLaunchResult<Launch::FLaunchEngineConfig> Launch::ParseEngineConfig ( const FCommandLine &InCommandLine ) noexcept
{
    if ( TLaunchResult<void> Validated = ValidateOptions( InCommandLine ); not Validated )
    {
        return MakeUnexpected( Validated.Error() );
    }

    if ( InCommandLine.HasSwitch( "help" ) )
    {
        PrintUsage( InCommandLine );
        return LaunchFailure( ELaunchStage::PreInit, ELaunchErrorCode::None, "help requested" );
    }

    return BindOptions( InCommandLine, FLaunchEngineConfig{} ).AndThen( &ValidateEngineConfig );
}

} // namespace Lumen
