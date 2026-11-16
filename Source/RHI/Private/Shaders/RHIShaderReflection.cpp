/**
 * @file RHIShaderReflection.cpp
 * @brief Implements shader reflection validation.
 */

#include <Shaders/RHIShaderReflection.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIShaderReflection::Validate () const noexcept
{
    for ( USize First = 0U; First < Bindings.size(); ++First )
    {
        if ( Bindings[First].Count == 0U )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Shader, "Reflected binding count is zero" ) );
        }
        for ( USize Second = First + 1U; Second < Bindings.size(); ++Second )
        {
            if ( Bindings[First].Slot == Bindings[Second].Slot )
            {
                return TUnexpected<FRHIError>(
                    MakeRHIInvalidArgument( ERHIErrorCondition::Shader, "Duplicate reflected binding slot" ) );
            }
        }
    }
    return {};
}
