/**
 * @file LaunchEngineTests.cpp
 * @brief Functional tests for the top-level GuardedMain entry point.
 */

#include <Engine/LaunchEngine.hpp>
#include <Engine/LaunchEngineLoop.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Private
 */

namespace
{

    [[nodiscard]] TVector<const AnsiChar *> MakeArgv ( const std::initializer_list<const AnsiChar *> InArgs ) noexcept
    {
        return TVector<const AnsiChar *>( InArgs );
    }

} // namespace

/**
 * Private
 */

LUMEN_TEST( GuardedMain, SucceedsWithHeadlessAndMaxFrames )
{
    const auto Arguments = MakeArgv( { "Lumen", "-headless", "-maxframes=1" } );

    Launch::FLaunchEngineLoop::ResetExitRequest();
    const Int32 ExitCode = Launch::GuardedMain( static_cast<Int32>( Arguments.size() ), Arguments.data() );

    LUMEN_CHECK_EQ( ExitCode, LUMEN_EXIT_SUCCESS );
}

LUMEN_TEST( GuardedMain, FailsWithInvalidConfiguration )
{
    const auto Arguments = MakeArgv( { "Lumen", "-fixedstep=0" } );

    Launch::FLaunchEngineLoop::ResetExitRequest();
    const Int32 ExitCode = Launch::GuardedMain( static_cast<Int32>( Arguments.size() ), Arguments.data() );

    LUMEN_CHECK_EQ( ExitCode, static_cast<Int32>( Launch::ELaunchErrorCode::InvalidConfiguration ) );
}

LUMEN_TEST( GuardedMain, FailsWithUnknownOption )
{
    const auto Arguments = MakeArgv( { "Lumen", "-unknownoption" } );

    Launch::FLaunchEngineLoop::ResetExitRequest();
    const Int32 ExitCode = Launch::GuardedMain( static_cast<Int32>( Arguments.size() ), Arguments.data() );

    LUMEN_CHECK_EQ( ExitCode, static_cast<Int32>( Launch::ELaunchErrorCode::InvalidArgument ) );
}

} // namespace Lumen
