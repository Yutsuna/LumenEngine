/*
- @file LaunchEngine.cpp
- @brief Launches the Lumen Engine
 */

#include <Core/CoreDefinitions.hpp>

#include <Engine/LaunchEngine.hpp>
#include <Engine/LaunchEngineLoop.hpp>
#include <Engine/LaunchProcessGuard.hpp>

#include <Logging/Logger.hpp>

namespace
{

LUMEN_LOG_DEFINE_CATEGORY( LogLaunch, "Launch" )

Lumen::Launch::TLaunchResult<Lumen::Int32> ReportFatalError ( const Lumen::Launch::FLaunchError &InError ) noexcept
{
    if ( InError.Code == Lumen::Launch::ELaunchErrorCode::None )
    {
        return LUMEN_EXIT_SUCCESS;
    }
    LUMEN_LOG_ERROR( LogLaunch, "Fatal error during {}: {:.{}} (code {})", ToCString( InError.Stage ), InError.Message.data(),
                     InError.Message.size(), InError.ToExitCode() );

    return InError.ToExitCode();
}

constexpr Lumen::Int32 ToExitCode ( const Lumen::Launch::FRunSummary LUMEN_UNUSED &RunSummary ) noexcept
{
    return LUMEN_EXIT_SUCCESS;
}

} // namespace

Lumen::Int32 Lumen::Launch::GuardedMain ( const Int32 InArgc, const AnsiChar *const *InArgv ) noexcept
{
    const FLaunchProcessGuard ProcessGuard;
    const Int32 ExitCode = FLaunchEngineLoop::PreInit( FCommandLine( InArgc, InArgv ) )
                               .AndThen( &FLaunchEngineLoop::Init )
                               .AndThen( &FLaunchEngineLoop::Run )
                               .Transform( &ToExitCode )
                               .OrElse( &ReportFatalError )
                               .ValueOr( LUMEN_EXIT_FAILURE );

    return ExitCode;
}
