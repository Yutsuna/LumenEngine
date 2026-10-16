/// @file TestRunner.cpp
/// @brief Standard entry-point for Lumen module unit test suites.

#include <Tests/UnitTest.hpp>

#include <Logging/Logger.hpp>

Lumen::Int32 main ( const Lumen::Int32 argc, const Lumen::AnsiChar *argv[] )
{
    Lumen::FStringView SuiteFilter{};
    if ( argc > 1 )
    {
        SuiteFilter = argv[1];
    }

    struct FGuardedScope final
    {
        constexpr FGuardedScope () noexcept
        {
            Lumen::FLogger::GetInstance().Initialize();
        };
        ~FGuardedScope () noexcept
        {
            Lumen::FLogger::GetInstance().Shutdown();
        }
    } GuardedScope{};

    const Lumen::Tests::FUnitTestTestRunSummary Summary = Lumen::Tests::FUnitTestRegistry::GetInstance().Run( SuiteFilter );

    return Summary.IsSuccessful() ? 0 : 84;
}