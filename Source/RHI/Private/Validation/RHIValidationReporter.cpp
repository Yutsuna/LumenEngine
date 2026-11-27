/**
 * @file RHIValidationReporter.cpp
 * @brief Implements validation diagnostic filtering and rejection.
 */

#include "RHIValidationReporter.hpp"
#include <HAL/LockGuard.hpp>
#include <RHI/Debug/IRHIDebugSink.hpp>
#include <RHI/Types/IRHIObject.hpp>

/**
 * Public
 */

Lumen::TSharedRef<Lumen::RHI::FRHIValidationReporter>
Lumen::RHI::FRHIValidationReporter::Create ( const FRHIValidationDesc &InDesc ) noexcept
{
    return MakeShared<FRHIValidationReporter>( InDesc );
}

Lumen::RHI::FRHIValidationReporter::FRHIValidationReporter ( const FRHIValidationDesc &InDesc ) noexcept : Desc( InDesc )
{
    /* */
}

void Lumen::RHI::FRHIValidationReporter::Report ( const ERHIValidationMessageCode InCode,
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
        const HAL::TLockGuard<HAL::FPlatformMutex> Lock( Mutex );
        if ( SeenMessages.contains( Key ) )
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

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationReporter::Reject ( const ERHIValidationMessageCode InCode,
                                                                      const ERHIErrorCondition InCondition,
                                                                      const FStringView InMessage,
                                                                      const IRHIObject *InObject ) const noexcept
{
    Report( InCode, InMessage, InObject );
    return TUnexpected<FRHIError>( MakeRHIValidationFailed( InCondition, InMessage ) );
}

const Lumen::RHI::FRHIValidationDesc &Lumen::RHI::FRHIValidationReporter::GetDesc () const noexcept
{
    return Desc;
}
