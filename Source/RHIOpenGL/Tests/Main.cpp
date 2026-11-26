/**
 * @file Main.cpp
 * @brief Runs OpenGL tests and the M1 portable conformance validation matrix.
 */

#include <Core/CoreDefinitions.hpp>

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <Logging/Logger.hpp>
#include <Tests/UnitTest.hpp>

Lumen::Int32 main ( const Lumen::Int32 Argc, const Lumen::AnsiChar *Argv[] )
{
    Lumen::FStringView SuiteFilter{};
    if ( Argc > 1 )
    {
        SuiteFilter = Argv[1];
    }

    struct FGuardedScope final
    {
        constexpr FGuardedScope () noexcept
        {
            Lumen::FLogger::GetInstance().Initialize();
        }
        ~FGuardedScope () noexcept
        {
            Lumen::FLogger::GetInstance().Shutdown();
        }
    } GuardedScope{};

    Lumen::RHI::Tests::GetRHIConformanceDeviceFactory().SetDefaultValidation( false );
    const Lumen::Tests::FUnitTestTestRunSummary StandardSummary =
        Lumen::Tests::FUnitTestRegistry::GetInstance().Run( SuiteFilter );
    Lumen::Bool bSuccessful = StandardSummary.IsSuccessful();
    if ( SuiteFilter.empty() )
    {
        Lumen::RHI::Tests::GetRHIConformanceDeviceFactory().SetDefaultValidation( true );
        const Lumen::Tests::FUnitTestTestRunSummary ValidationSummary =
            Lumen::Tests::FUnitTestRegistry::GetInstance().Run( "Conformance" );
        bSuccessful = bSuccessful and ValidationSummary.IsSuccessful();
    }
    Lumen::RHI::Tests::GetRHIConformanceDeviceFactory().SetDefaultValidation( false );
    return bSuccessful ? LUMEN_EXIT_SUCCESS : LUMEN_EXIT_FAILURE;
}
