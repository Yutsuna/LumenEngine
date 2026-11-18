/**
 * @file UnitTest.cpp
 * @brief Unit test framework implementation.
 */

#include <Tests/UnitTest.hpp>
#include <Tests/UnitTestAssertionFailure.hpp>

#include <HAL/PlatformTime.hpp>
#include <Logging/Logger.hpp>
#include <String/String.hpp>

#include <exception>
#include <format>

namespace Lumen
{

LUMEN_LOG_DEFINE_CATEGORY( LogUnitTest, "UnitTest" )

namespace
{

    constexpr const AnsiChar *Green = "\033[32m";
    constexpr const AnsiChar *Red   = "\033[31m";
    constexpr const AnsiChar *Grey  = "\033[90m";
    constexpr const AnsiChar *Reset = "\033[0m";

    constexpr FStringView UnitTestKindToString ( Tests::EUnitTestKind Kind ) noexcept;
    void RunOne ( Tests::FUnitTestTestRunSummary &InUnitTestSummary,
                  const Tests::FUnitTestMetadata &InTest,
                  Tests::EUnitTestOutputMode OutputMode );

} // namespace

/**
 * Public
 */

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
    return Run( InSuiteFilter, InKindFilter, EUnitTestOutputMode::Compact );
}

Tests::FUnitTestTestRunSummary Tests::FUnitTestRegistry::Run ( const FStringView InSuiteFilter,
                                                               TOptional<EUnitTestKind> InKindFilter,
                                                               const EUnitTestOutputMode OutputMode ) const
{
    FUnitTestTestRunSummary UnitTestSummary{};
    const Float64 StartTime = HAL::FPlatformTime::Seconds();

    if ( OutputMode == EUnitTestOutputMode::Detailed )
    {
        LUMEN_LOG_DISPLAY( LogUnitTest, "[================] Running registered test suites..." );
    }

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

        RunOne( UnitTestSummary, Test, OutputMode );
    }

    UnitTestSummary.DurationSeconds = HAL::FPlatformTime::Seconds() - StartTime;

    if ( OutputMode == EUnitTestOutputMode::Detailed )
    {
        LUMEN_LOG_DISPLAY( LogUnitTest, "[================] {} tests executed across {:.4f} s.", UnitTestSummary.TotalExecuted,
                           UnitTestSummary.DurationSeconds );
        LUMEN_LOG_DISPLAY( LogUnitTest, "[  PASSED    ] {} tests.", UnitTestSummary.PassedCount );

        if ( UnitTestSummary.FailedCount > 0 )
        {
            LUMEN_LOG_ERROR( LogUnitTest, "[  FAILED    ] {} tests.", UnitTestSummary.FailedCount );
        }
    }
    else
    {
        const AnsiChar *FailedColor = UnitTestSummary.FailedCount > 0 ? Red : Grey;

        LUMEN_LOG_DISPLAY( LogUnitTest, "{}{} passed{}, {}{} failed{} in {:.3f} ms.", Green, UnitTestSummary.PassedCount, Reset,
                           FailedColor, UnitTestSummary.FailedCount, Reset, UnitTestSummary.DurationSeconds * 1000.0 );
    }

    return UnitTestSummary;
}

USize Tests::FUnitTestRegistry::GetRegisteredCount () const noexcept
{
    return RegisteredTests.size();
}

/**
 * Private
 */

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

    void RunOne ( Tests::FUnitTestTestRunSummary &InUnitTestSummary,
                  const Tests::FUnitTestMetadata &InTest,
                  const Tests::EUnitTestOutputMode OutputMode )
    {
        ++InUnitTestSummary.TotalExecuted;
        const FString FullyQualifiedName = std::format( "{}.{}", InTest.SuiteName, InTest.TestName );
        const FStringView KindLabel      = UnitTestKindToString( InTest.Kind );

        if ( OutputMode == Tests::EUnitTestOutputMode::Detailed )
        {
            LUMEN_LOG_DISPLAY( LogUnitTest, "[ RUN        ] [{}] {}", KindLabel, FullyQualifiedName );
        }

        const Float64 TestStartTime = HAL::FPlatformTime::Seconds();

        try
        {
            InTest.ExecutionFunction();
            const Float64 Elapsed = HAL::FPlatformTime::Seconds() - TestStartTime;

            ++InUnitTestSummary.PassedCount;

            if ( OutputMode == Tests::EUnitTestOutputMode::Detailed )
            {
                LUMEN_LOG_DISPLAY( LogUnitTest, "[       OK   ] {} ({:.3f} ms)", FullyQualifiedName, Elapsed * 1000.0 );
            }
            else if ( OutputMode == Tests::EUnitTestOutputMode::Compact )
            {
                LUMEN_LOG_DISPLAY( LogUnitTest, "{}[ OK ]{} {} ({:.3f} ms)", Green, Reset, FullyQualifiedName, Elapsed * 1000.0 );
            }
        }
        catch ( const Tests::FUnitTestAssertionFailure &Failure )
        {
            const Float64 Elapsed = HAL::FPlatformTime::Seconds() - TestStartTime;
            ++InUnitTestSummary.FailedCount;

            const auto &Location = Failure.GetLocation();

            if ( OutputMode == Tests::EUnitTestOutputMode::Detailed )
            {
                LUMEN_LOG_ERROR( LogUnitTest, "[   FAILED   ] {} ({:.3f} ms)", FullyQualifiedName, Elapsed * 1000.0 );
            }
            else
            {
                LUMEN_LOG_ERROR( LogUnitTest, "{}[ FAIL ]{} {} ({:.3f} ms)", Red, Reset, FullyQualifiedName, Elapsed * 1000.0 );
            }

            LUMEN_LOG_ERROR( LogUnitTest, "               -> Assertion Failure: {}", Failure.what() );
            LUMEN_LOG_ERROR( LogUnitTest, "               -> Location: {}:{}:{}", Location.file_name(), Location.line(),
                             Location.column() );
        }
        catch ( const std::exception &Exception )
        {
            const Float64 Elapsed = HAL::FPlatformTime::Seconds() - TestStartTime;
            ++InUnitTestSummary.FailedCount;

            if ( OutputMode == Tests::EUnitTestOutputMode::Detailed )
            {
                LUMEN_LOG_ERROR( LogUnitTest, "[   CRASH    ] {} ({:.3f} ms)", FullyQualifiedName, Elapsed * 1000.0 );
            }
            else
            {
                LUMEN_LOG_ERROR( LogUnitTest, "{}[CRASH]{} {} ({:.3f} ms)", Red, Reset, FullyQualifiedName, Elapsed * 1000.0 );
            }

            LUMEN_LOG_ERROR( LogUnitTest, "               -> Unhandled std::exception: {}", Exception.what() );
        }
        catch ( ... )
        {
            const Float64 Elapsed = HAL::FPlatformTime::Seconds() - TestStartTime;
            ++InUnitTestSummary.FailedCount;

            if ( OutputMode == Tests::EUnitTestOutputMode::Detailed )
            {
                LUMEN_LOG_ERROR( LogUnitTest, "[   CRASH    ] {} ({:.3f} ms)", FullyQualifiedName, Elapsed * 1000.0 );
            }
            else
            {
                LUMEN_LOG_ERROR( LogUnitTest, "{}[CRASH]{} {} ({:.3f} ms)", Red, Reset, FullyQualifiedName, Elapsed * 1000.0 );
            }

            LUMEN_LOG_ERROR( LogUnitTest, "               -> Unhandled non-standard exception or SEH occurred." );
        }
    }

} // namespace

} // namespace Lumen
