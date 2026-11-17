/**
 * @file RHICapabilities.cpp
 * @brief Implements capability queries and validation.
 */

#include <Types/RHICapabilities.hpp>

Lumen::Bool Lumen::FRHICapabilities::Supports ( const ERHICapabilityFlags InRequired ) const noexcept
{
    return ( Features & InRequired ) == InRequired;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHICapabilities::Validate () const noexcept
{
    constexpr UInt64 KnownMask = ( 1ULL << 18U ) - 1ULL;
    const UInt64 RawFeatures   = static_cast<UInt64>( Features );
    if ( ( RawFeatures & ~KnownMask ) != 0ULL )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( Unsupported, ERHIErrorCondition::Capability, "Unknown capability bits are set" ) );
    }
    return {};
}
