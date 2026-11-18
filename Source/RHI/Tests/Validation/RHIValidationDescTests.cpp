/**
 * @file RHIValidationDescTests.cpp
 * @brief Tests validation levels, flags, and diagnostic policies.
 */
#include "Validation/RHIValidationReporter.hpp"
#include "Validation/RHIValidationTestUtils.hpp"
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIValidationDesc, ValidatesLevelsFlagsAndSeverity )
    {
        FRHIValidationDesc Desc;
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        LUMEN_CHECK_TRUE( Desc.IsEnabled( ERHIValidationFlags::Bindings ) );
        Desc.Level = ERHIValidationLevel::Basic;
        LUMEN_CHECK_TRUE( Desc.IsEnabled( ERHIValidationFlags::Commands ) );
        LUMEN_CHECK_FALSE( Desc.IsEnabled( ERHIValidationFlags::Presentation ) );
        Desc.Level = ERHIValidationLevel::Disabled;
        LUMEN_CHECK_FALSE( Desc.IsEnabled( ERHIValidationFlags::Commands ) );
        Desc.Level = ERHIValidationLevel::Count;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Level = ERHIValidationLevel::Full;
        Desc.Flags = static_cast<ERHIValidationFlags>( 1U << 31U );
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Flags           = ERHIValidationFlags::All;
        Desc.MinimumSeverity = ERHIDebugSeverity::Count;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }

    LUMEN_TEST( RHIValidationDesc, FiltersDeduplicatesAndEscalatesDiagnostics )
    {
        Tests::FRecordingDebugSink Sink;
        FRHIValidationDesc Desc;
        Desc.Sink            = &Sink;
        Desc.MinimumSeverity = ERHIDebugSeverity::Error;
        Desc.bFatalOnError   = true;
        FRHIValidationReporter Reporter( Desc );
        Reporter.Report( ERHIValidationMessageCode::ObjectLeak, "hidden", nullptr, ERHIDebugSeverity::Warning );
        Reporter.Report( ERHIValidationMessageCode::InvalidBinding, "visible" );
        Reporter.Report( ERHIValidationMessageCode::InvalidBinding, "duplicate" );
        LUMEN_CHECK_EQ( Sink.GetCount(), 1U );
        const TVector<FRHIDebugMessage> Messages = Sink.GetMessages();
        LUMEN_CHECK_EQ( Messages[0].Severity, ERHIDebugSeverity::Fatal );
        LUMEN_CHECK_EQ( Messages[0].Category, ERHIDebugCategory::Validation );
        Desc.bDeduplicateMessages = false;
        FRHIValidationReporter Unfiltered( Desc );
        Unfiltered.Report( ERHIValidationMessageCode::InvalidBinding, "first" );
        Unfiltered.Report( ERHIValidationMessageCode::InvalidBinding, "second" );
        LUMEN_CHECK_EQ( Sink.GetCount(), 3U );
    }
} // namespace
} // namespace Lumen
