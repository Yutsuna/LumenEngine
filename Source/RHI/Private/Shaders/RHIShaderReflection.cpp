/**
 * @file RHIShaderReflection.cpp
 * @brief Implements shader reflection validation.
 */

#include <RHI/Shaders/RHIShaderReflection.hpp>
Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIShaderReflection::Validate () const noexcept
{
    for ( USize First = 0U; First < Bindings.size(); ++First )
    {
        if ( Bindings[First].Count == 0U )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidShader( "Reflected binding count is zero" ) );
        }
        for ( USize Second = First + 1U; Second < Bindings.size(); ++Second )
        {
            if ( Bindings[First].Slot == Bindings[Second].Slot )
            {
                return TUnexpected<FRHIError>( MakeRHIInvalidShader( "Duplicate reflected binding slot" ) );
            }
        }
    }
    return {};
}
