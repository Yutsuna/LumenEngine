/**
 * @file RHICapabilities.cpp
 * @brief Implements capability queries and validation.
 */

#include <RHI/Types/RHICapabilities.hpp>

Lumen::Bool Lumen::RHI::FRHICapabilities::Supports ( const ERHICapabilityFlags InRequired ) const noexcept
{
    return ( Features & InRequired ) == InRequired;
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHICapabilities::Validate () const noexcept
{
    constexpr UInt64 KnownMask = ( 1ULL << 18U ) - 1ULL;
    const UInt64 RawFeatures   = static_cast<UInt64>( Features );
    if ( ( RawFeatures & ~KnownMask ) != 0ULL )
    {
        return TUnexpected<FRHIError>( MakeRHIUnsupported( ERHIErrorCondition::Capability, "Unknown capability bits are set" ) );
    }
    return {};
}
