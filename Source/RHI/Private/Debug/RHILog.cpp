/**
 * @file RHILog.cpp
 * @brief Implements RHI diagnostic logging through Core.
 */

#include "RHILog.hpp"

const Lumen::FLogCategory &Lumen::RHI::GetRHILogCategory () noexcept
{
    static const FLogCategory Category( static_cast<FStringView>( "RHI" ) );
    return Category;
}

Lumen::ELogVerbosity::Type Lumen::RHI::ToRHILogVerbosity ( const ERHIDebugSeverity InSeverity ) noexcept
{
    switch ( InSeverity )
    {
    case ERHIDebugSeverity::Info:
        return ELogVerbosity::Info;
    case ERHIDebugSeverity::Warning:
        return ELogVerbosity::Warning;
    case ERHIDebugSeverity::Error: /** NOLINT(bugprone-branch-clone) */
        return ELogVerbosity::Error;
    case ERHIDebugSeverity::Fatal: /** NOLINT(bugprone-branch-clone) */
        return ELogVerbosity::Error;
    default:
        return ELogVerbosity::Info;
    }
}

void Lumen::RHI::LogRHIDebugMessage ( const FRHIDebugMessage &InMessage ) noexcept
{
    LUMEN_LOG( GetRHILogCategory(), ToRHILogVerbosity( InMessage.Severity ), "{}", InMessage.Message );
}
