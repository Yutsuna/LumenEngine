/// @file TestRunner.cpp
/// @brief Standard entry-point for Lumen module unit test suites.

#include <Tests/UnitTest.hpp>

Lumen::Int32 main ( const Lumen::Int32 argc, const Lumen::AnsiChar *argv[] )
{
    Lumen::FStringView SuiteFilter{};
    if ( argc > 1 )
    {
        SuiteFilter = argv[1];
    }

    const Lumen::Tests::FUnitTestTestRunSummary Summary = Lumen::Tests::FUnitTestRegistry::GetInstance().Run( SuiteFilter );

    return Summary.IsSuccessful() ? 0 : 1;
}