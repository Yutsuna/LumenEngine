/**
 * @file RHIShaderDesc.cpp
 * @brief Implements shader descriptor validation.
 */

#include <RHI/Shaders/RHIShaderDesc.hpp>
Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIShaderDesc::Validate () const noexcept
{
    const UInt32 StageBits = static_cast<UInt32>( Stage );
    if ( StageBits == 0U or ( StageBits & ( StageBits - 1U ) ) != 0U or EntryPoint.empty() or
         not IsKnownRHIShaderFormat( Format ) or not Reflection.Validate() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidShader( "Invalid shader description" ) );
    }
    return {};
}
