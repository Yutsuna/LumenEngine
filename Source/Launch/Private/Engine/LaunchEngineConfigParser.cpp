/// @file LaunchEngineConfigParser.cpp
/// @brief Parse the Command Line then Validate the configuration.

#include <Engine/LaunchEngineConfig.hpp>
#include <Engine/LaunchEngineConfigParser.hpp>

#include <cmath>
#include <type_traits>
#include <utility>

namespace Lumen
{

namespace
{
    /// @brief Prevents `-fixedstep=1e-9` from freezing the engine in the catch-up loop.
    constexpr Float64 MaxFixedStepsPerFrame = 1024.0;

    [[nodiscard]] constexpr TUnexpected<Launch::FLaunchError> InvalidConfig ( const FStringView InMessage ) noexcept
    {
        return Launch::LaunchFailure( Launch::ELaunchStage::PreInit, Launch::ELaunchErrorCode::InvalidConfiguration, InMessage );
    }

    /// @brief -Key=Value -> Config.*Member. Absent key keeps the current value, malformed fails.
    /// @tparam Member The numeric member of FLaunchEngineConfig to set.
    template <auto Member>
    [[nodiscard]] auto WithOption ( const Launch::FCommandLine &InCommandLine, const FStringView InKey ) noexcept
    {
        using FValue = std::remove_cvref_t<decltype( std::declval<Launch::FLaunchEngineConfig &>().*Member )>;
        static_assert( Launch::Concepts::CCommandLineNumber<FValue>,
                       "WithOption only sets numeric members: use WithSwitch for Bool." );

        return [&InCommandLine,
                InKey] ( Launch::FLaunchEngineConfig InConfig ) noexcept -> Launch::TLaunchResult<Launch::FLaunchEngineConfig>
        {
            return InCommandLine.GetNumberOr<FValue>( InKey, InConfig.*Member )
                .Transform(
                    [&InConfig] ( const FValue InValue ) noexcept -> Launch::FLaunchEngineConfig
                    {
                        InConfig.*Member = InValue;
                        return InConfig;
                    } );
        };
    }

    /// @brief -Name -> Config.*Member = true. Can only be used for Bool members.
    /// @tparam Member The Bool member of FLaunchEngineConfig to set.
    template <auto Member>
    [[nodiscard]] auto WithSwitch ( const Launch::FCommandLine &InCommandLine, const FStringView InName ) noexcept
    {
        static_assert(
            Concepts::CIsSame<std::remove_cvref_t<decltype( std::declval<Launch::FLaunchEngineConfig &>().*Member )>, Bool>,
            "WithSwitch only sets Bool members: use WithOption for numbers." );

        return [&InCommandLine,
                InName] ( Launch::FLaunchEngineConfig InConfig ) noexcept -> Launch::TLaunchResult<Launch::FLaunchEngineConfig>
        {
            InConfig.*Member = InConfig.*Member or InCommandLine.HasSwitch( InName );
            return InConfig;
        };
    }
} // namespace

Launch::TLaunchResult<Launch::FLaunchEngineConfig> Launch::ValidateEngineConfig ( const FLaunchEngineConfig &InConfig ) noexcept
{
    if ( not std::isfinite( InConfig.FixedTimeStep ) or InConfig.FixedTimeStep <= 0.0 ) [[unlikely]]
    {
        return InvalidConfig( "-fixedstep must be a finite value greater than 0" );
    }

    if ( not std::isfinite( InConfig.MaxFrameTime ) or InConfig.MaxFrameTime < InConfig.FixedTimeStep ) [[unlikely]]
    {
        return InvalidConfig( "-maxframetime must be finite and not lower than -fixedstep" );
    }

    if ( InConfig.MaxFrameTime > InConfig.FixedTimeStep * MaxFixedStepsPerFrame ) [[unlikely]]
    {
        return InvalidConfig( "-maxframetime / -fixedstep exceeds the allowed number of fixed steps per frame" );
    }

    if ( not std::isfinite( InConfig.MaxFPS ) or InConfig.MaxFPS < 0.0 ) [[unlikely]]
    {
        return InvalidConfig( "-fps must be a finite value greater than or equal to 0" );
    }

    return InConfig;
}

Launch::TLaunchResult<Launch::FLaunchEngineConfig> Launch::ParseEngineConfig ( const FCommandLine &InCommandLine ) noexcept
{
    return TLaunchResult( FLaunchEngineConfig{} )
        .AndThen( WithOption<&FLaunchEngineConfig::FixedTimeStep>( InCommandLine, "fixedstep" ) )
        .AndThen( WithOption<&FLaunchEngineConfig::MaxFrameTime>( InCommandLine, "maxframetime" ) )
        .AndThen( WithOption<&FLaunchEngineConfig::MaxFPS>( InCommandLine, "fps" ) )
        .AndThen( WithOption<&FLaunchEngineConfig::MaxFrames>( InCommandLine, "maxframes" ) )
        .AndThen( WithSwitch<&FLaunchEngineConfig::bHeadless>( InCommandLine, "headless" ) )
        .AndThen( &ValidateEngineConfig );
}

} // namespace Lumen
