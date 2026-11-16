/**
 * @file RHILogTests.cpp
 * @brief Tests adaptation of RHI diagnostics to Core logging.
 */

#include <Debug/RHILog.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHILog, MapsSeverityWithoutEscalatingFatalDiagnostics )
    {
        LUMEN_CHECK_TRUE( GetRHILogCategory().CategoryName == "RHI" );
        LUMEN_CHECK_EQ( ToRHILogVerbosity( ERHIDebugSeverity::Info ), ELogVerbosity::Info );
        LUMEN_CHECK_EQ( ToRHILogVerbosity( ERHIDebugSeverity::Warning ), ELogVerbosity::Warning );
        LUMEN_CHECK_EQ( ToRHILogVerbosity( ERHIDebugSeverity::Error ), ELogVerbosity::Error );
        LUMEN_CHECK_EQ( ToRHILogVerbosity( ERHIDebugSeverity::Fatal ), ELogVerbosity::Error );
        LUMEN_CHECK_EQ( ToRHILogVerbosity( ERHIDebugSeverity::Count ), ELogVerbosity::Info );
    }

    LUMEN_TEST( RHILog, EmitsMessagesThroughTheRHICategoryOnly )
    {
        const FRHIDebugMessage Message( ERHIDebugSeverity::Info, ERHIDebugCategory::General, 0U, "RHI log adapter test" );
        LogRHIDebugMessage( Message );
        LUMEN_CHECK_TRUE( GetRHILogCategory().CategoryName == "RHI" );
    }

} // namespace
} // namespace Lumen
