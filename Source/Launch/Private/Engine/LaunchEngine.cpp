/// @file LaunchEngine.cpp
/// @brief Launches the Lumen Engine

#include <Core/CoreDefinitions.hpp>

#include <Engine/LaunchEngine.hpp>
#include <Engine/LaunchEngineLoop.hpp>

#include <Logging/Logger.hpp>

namespace
{

LUMEN_LOG_DEFINE_CATEGORY( LogLaunch, "Launch" )

[[nodiscard]] Lumen::Launch::TLaunchResult<Lumen::Int32> ReportFatalError ( const Lumen::Launch::FLaunchError &InError ) noexcept
{
    LUMEN_LOG_ERROR( LogLaunch, "Fatal error during {}: {:.{}} (code {})", ToCString( InError.Stage ), InError.Message.data(),
                     static_cast<int>( InError.Message.size() ), InError.ToExitCode() );

    return InError.ToExitCode();
}

[[nodiscard]] constexpr Lumen::Int32 ToExitCode ( const Lumen::Launch::FRunSummary __attribute__( ( unused ) ) &
                                                  RunSummary ) noexcept
{
    return LUMEN_EXIT_SUCCESS;
}

} // namespace

Lumen::Int32 Lumen::Launch::GuardedMain ( const Int32 InArgc, const AnsiChar *const *InArgv ) noexcept
{
    const Int32 ExitCode = FLaunchEngineLoop::PreInit( FCommandLine( InArgc, InArgv ) )
                               .AndThen( &FLaunchEngineLoop::Init )
                               .AndThen( &FLaunchEngineLoop::Run )
                               .Transform( &ToExitCode )
                               .OrElse( &ReportFatalError )
                               .ValueOr( LUMEN_EXIT_FAILURE );

    return ExitCode;
}
