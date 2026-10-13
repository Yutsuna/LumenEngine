/// @file UnitTest.cpp
/// @brief Unit test framework implementation.

#include <Tests/UnitTest.hpp>
#include <Tests/UnitTestAssertionFailure.hpp>

#include <HAL/PlatformTime.hpp>
#include <String/String.hpp>

#include <print>

namespace Lumen
{

namespace
{

    constexpr FStringView UnitTestKindToString ( const Tests::EUnitTestKind Kind ) noexcept
    {
        switch ( Kind )
        {
        case Tests::EUnitTestKind::Functional:
            return "FUNCTIONAL";
        case Tests::EUnitTestKind::Integration:
            return "INTEGRATION";
        default:
            return "UNKNOWN";
        }
    }

} // namespace

Tests::FUnitTestRegistry &Tests::FUnitTestRegistry::GetInstance () noexcept
{
    static FUnitTestRegistry Instance;
    return Instance;
}

void Tests::FUnitTestRegistry::Register ( FUnitTestMetadata const &Metadata )
{
    RegisteredTests.push_back( Metadata );
}

Tests::FUnitTestTestRunSummary Tests::FUnitTestRegistry::Run ( const FStringView InSuiteFilter,
                                                               TOptional<EUnitTestKind> InKindFilter ) const
{
    FUnitTestTestRunSummary UnitTestSummary{};
    const Float64 StartTime = HAL::FPlatformTime::Seconds();

    std::println( "\n[================] Running registered test suites..." );

    for ( const auto &Test : RegisteredTests )
    {
        if ( not InSuiteFilter.empty() and not Test.SuiteName.contains( InSuiteFilter ) )
        {
            continue;
        }

        if ( InKindFilter.HasValue() and Test.Kind != *InKindFilter )
        {
            continue;
        }

        ++UnitTestSummary.TotalExecuted;
        const FString FullyQualifiedName = std::format( "{}.{}", Test.SuiteName, Test.TestName );
        const FStringView KindLabel      = UnitTestKindToString( Test.Kind );

        std::println( "[ RUN        ] [{}] {}", KindLabel, FullyQualifiedName );
        const Float64 TestStartTime = HAL::FPlatformTime::Seconds();

        try
        {
            Test.ExecutionFunction();
            const Float64 Elapsed = HAL::FPlatformTime::Seconds() - TestStartTime;

            ++UnitTestSummary.PassedCount;
            std::println( "[       OK   ] {} ({:.3f} ms)", FullyQualifiedName, Elapsed * 1000.0 );
        }
        catch ( const FUnitTestAssertionFailure &Failure )
        {
            const Float64 Elapsed = HAL::FPlatformTime::Seconds() - TestStartTime;
            ++UnitTestSummary.FailedCount;

            std::println( stderr, "[   FAILED   ] {} ({:.3f} ms)", FullyQualifiedName, Elapsed * 1000.0 );
            std::println( stderr, "               -> Assertion Failure: {}", Failure.what() );
            std::println( stderr, "               -> Location: {}:{}:{}", Failure.GetLocation().file_name(),
                          Failure.GetLocation().line(), Failure.GetLocation().column() );
        }
        catch ( const std::exception &Exception )
        {
            const Float64 Elapsed = HAL::FPlatformTime::Seconds() - TestStartTime;
            ++UnitTestSummary.FailedCount;

            std::println( stderr, "[   CRASH    ] {} ({:.3f} ms)", FullyQualifiedName, Elapsed * 1000.0 );
            std::println( stderr, "               -> Unhandled std::exception: {}", Exception.what() );
        }
        catch ( ... )
        {
            const Float64 Elapsed = HAL::FPlatformTime::Seconds() - TestStartTime;
            ++UnitTestSummary.FailedCount;

            std::println( stderr, "[   CRASH    ] {} ({:.3f} ms)", FullyQualifiedName, Elapsed * 1000.0 );
            std::println( stderr, "               -> Unhandled non-standard exception or SEH occurred." );
        }
    }

    UnitTestSummary.DurationSeconds = HAL::FPlatformTime::Seconds() - StartTime;

    std::println( "[================] {} tests executed across {:.4f} s.", UnitTestSummary.TotalExecuted,
                  UnitTestSummary.DurationSeconds );
    std::println( "[  PASSED    ] {} tests.", UnitTestSummary.PassedCount );

    if ( UnitTestSummary.FailedCount > 0 )
    {
        std::println( stderr, "[  FAILED    ] {} tests.", UnitTestSummary.FailedCount );
    }

    return UnitTestSummary;
}

USize Tests::FUnitTestRegistry::GetRegisteredCount () const noexcept
{
    return RegisteredTests.size();
}

} // namespace Lumen