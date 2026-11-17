/**
 * @file RHIValidationReporter.cpp
 * @brief Implements validation diagnostic filtering and rejection.
 */
#include "RHIValidationReporter.hpp"
#include <Debug/IRHIDebugSink.hpp>
#include <HAL/LockGuard.hpp>
#include <Types/IRHIObject.hpp>

Lumen::FRHIValidationReporter::FRHIValidationReporter ( const FRHIValidationDesc &InDesc ) noexcept : Desc( InDesc )
{
}

void Lumen::FRHIValidationReporter::Report ( const ERHIValidationMessageCode InCode,
                                             const FStringView InMessage,
                                             const IRHIObject *InObject,
                                             const ERHIDebugSeverity InSeverity ) const noexcept
{
    const ERHIDebugSeverity Severity =
        Desc.bFatalOnError and InSeverity >= ERHIDebugSeverity::Error ? ERHIDebugSeverity::Fatal : InSeverity;
    if ( Severity < Desc.MinimumSeverity or Desc.Sink == nullptr )
    {
        return;
    }
    const UInt64 ObjectValue = InObject == nullptr ? 0ULL : InObject->GetId().GetValue();
    const UInt64 Key         = ( static_cast<UInt64>( InCode ) << 32U ) ^ ObjectValue;
    if ( Desc.bDeduplicateMessages )
    {
        HAL::TLockGuard<HAL::FPlatformMutex> Lock( Mutex );
        if ( SeenMessages.find( Key ) != SeenMessages.end() )
        {
            return;
        }
        SeenMessages.emplace( Key, FString( InMessage ) );
    }
    FRHIDebugMessage Message( Severity, ERHIDebugCategory::Validation, static_cast<UInt32>( InCode ), InMessage );
    if ( InObject != nullptr )
    {
        Message.AddObject( InObject->GetId() );
    }
    Desc.Sink->Emit( Message );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationReporter::Reject ( const ERHIValidationMessageCode InCode,
                                                                                 const ERHIErrorCondition InCondition,
                                                                                 const FStringView InMessage,
                                                                                 const IRHIObject *InObject ) const noexcept
{
    Report( InCode, InMessage, InObject );
    return TUnexpected<FRHIError>( FRHIError( ERHIErrorCode::ValidationFailed, InCondition, InMessage ) );
}

const Lumen::FRHIValidationDesc &Lumen::FRHIValidationReporter::GetDesc () const noexcept
{
    return Desc;
}
