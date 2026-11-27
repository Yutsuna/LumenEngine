/**
 * @file RHICapabilities.cpp
 * @brief Implements capability queries and validation.
 */

#include <RHI/Types/RHICapabilities.hpp>

Lumen::Bool Lumen::RHI::FRHICapabilities::Supports ( const ERHICapabilityFlags InRequired ) const noexcept
{
    return ( Features & InRequired ) == InRequired;
}

Lumen::Bool Lumen::RHI::FRHICapabilities::Supports ( const ERHIShaderFormat InFormat ) const noexcept
{
    for ( const ERHIShaderFormat Format : ShaderFormats )
    {
        if ( Format == InFormat )
        {
            return true;
        }
    }
    return false;
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHICapabilities::Validate () const noexcept
{
    constexpr UInt64 KnownMask = ( 1ULL << 18U ) - 1ULL;
    const UInt64 RawFeatures   = static_cast<UInt64>( Features );
    if ( ( RawFeatures & ~KnownMask ) != 0ULL )
    {
        return TUnexpected<FRHIError>( MakeRHIUnsupported( ERHIErrorCondition::Capability, "Unknown capability bits are set" ) );
    }
    for ( USize First = 0U; First < ShaderFormats.size(); ++First )
    {
        if ( not IsKnownRHIShaderFormat( ShaderFormats[First] ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHIUnsupported( ERHIErrorCondition::Shader, "Unknown shader format is advertised" ) );
        }
        for ( USize Second = First + 1U; Second < ShaderFormats.size(); ++Second )
        {
            if ( ShaderFormats[First] == ShaderFormats[Second] )
            {
                return TUnexpected<FRHIError>(
                    MakeRHIUnsupported( ERHIErrorCondition::Shader, "Shader formats must not contain duplicates" ) );
            }
        }
    }
    return {};
}
