/// @file TestRunner.cpp
/// @brief Standard entry-point for Lumen module unit test suites.

#include <Tests/UnitTest.hpp>

int main ( const int argc, const char *argv[] )
{
    Lumen::FStringView SuiteFilter{};
    if ( argc > 1 )
    {
        SuiteFilter = argv[1];
    }

    const Lumen::Tests::FUnitTestTestRunSummary Summary = Lumen::Tests::FUnitTestRegistry::GetInstance().Run( SuiteFilter );

    return Summary.IsSuccessful() ? 0 : 1;
}
