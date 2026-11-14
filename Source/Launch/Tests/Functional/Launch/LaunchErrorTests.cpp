/**
 * @file LaunchErrorTests.cpp
 * @brief Functional tests for LaunchError and stage helpers.
 */

#include <Launch/LaunchError.hpp>

#include <Concepts/TriviallyCopyable.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Public
 */

LUMEN_TEST( FLaunchError_Traits, IsTriviallyCopyable )
{
    static_assert( Concepts::CTriviallyCopyable<Launch::FLaunchError> );
    LUMEN_CHECK_TRUE( Concepts::CTriviallyCopyable<Launch::FLaunchError> );
}

LUMEN_TEST( FLaunchError_ToExitCode, ReturnsNumericErrorCode )
{
    const Launch::FLaunchError Error{
        .Stage   = Launch::ELaunchStage::Init,
        .Code    = Launch::ELaunchErrorCode::ModuleCreationFailed,
        .Message = "test",
    };

    LUMEN_CHECK_EQ( Error.ToExitCode(), static_cast<Int32>( Launch::ELaunchErrorCode::ModuleCreationFailed ) );
}

LUMEN_TEST( FLaunchError_ToExitCode, MapsEveryErrorCode )
{
    constexpr Launch::ELaunchErrorCode Codes[] = { Launch::ELaunchErrorCode::None,
                                                   Launch::ELaunchErrorCode::MissingArgument,
                                                   Launch::ELaunchErrorCode::InvalidArgument,
                                                   Launch::ELaunchErrorCode::InvalidConfiguration,
                                                   Launch::ELaunchErrorCode::ModuleCreationFailed,
                                                   Launch::ELaunchErrorCode::OutOfMemory,
                                                   Launch::ELaunchErrorCode::Unknown };

    for ( const Launch::ELaunchErrorCode Code : Codes )
    {
        const Launch::FLaunchError Error{
            .Stage   = Launch::ELaunchStage::Run,
            .Code    = Code,
            .Message = "test",
        };

        LUMEN_CHECK_EQ( Error.ToExitCode(), static_cast<Int32>( Code ) );
    }

    /** INFO: None must stay 0 so that a "help requested" launch reports a successful exit code. */
    LUMEN_CHECK_EQ( static_cast<Int32>( Launch::ELaunchErrorCode::None ), 0 );
}

LUMEN_TEST( LaunchFailure_Builder, ProducesExpectedError )
{
    const auto Failure =
        Launch::LaunchFailure( Launch::ELaunchStage::PreInit, Launch::ELaunchErrorCode::InvalidArgument, "missing arg" );

    LUMEN_CHECK_EQ( Failure.Error().Stage, Launch::ELaunchStage::PreInit );
    LUMEN_CHECK_EQ( Failure.Error().Code, Launch::ELaunchErrorCode::InvalidArgument );
    LUMEN_CHECK_TRUE( Failure.Error().Message == "missing arg" );
}

LUMEN_TEST( LaunchFailure_Builder, TravelsThroughExpectedByValue )
{
    const Launch::TLaunchResult<void> First =
        Launch::LaunchFailure( Launch::ELaunchStage::Init, Launch::ELaunchErrorCode::ModuleCreationFailed, "boom" );

    const Launch::TLaunchResult<void> Second = First;

    LUMEN_CHECK_HAS_ERROR( Second );
    LUMEN_CHECK_EQ( Second.Error().Stage, Launch::ELaunchStage::Init );
    LUMEN_CHECK_EQ( Second.Error().Code, Launch::ELaunchErrorCode::ModuleCreationFailed );
    LUMEN_CHECK_TRUE( Second.Error().Message == "boom" );
}

LUMEN_TEST( ELaunchStage_ToCString, MapsEveryStage )
{
    LUMEN_CHECK_TRUE( std::string_view( Launch::ToCString( Launch::ELaunchStage::PreInit ) ) == "PreInit" );
    LUMEN_CHECK_TRUE( std::string_view( Launch::ToCString( Launch::ELaunchStage::Init ) ) == "Init" );
    LUMEN_CHECK_TRUE( std::string_view( Launch::ToCString( Launch::ELaunchStage::Run ) ) == "Run" );
}

} // namespace Lumen
