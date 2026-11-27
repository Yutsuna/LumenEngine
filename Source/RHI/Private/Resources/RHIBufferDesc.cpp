/**
 * @file RHIBufferDesc.cpp
 * @brief Implements buffer descriptor validation.
 */
#include <RHI/Resources/RHIBufferDesc.hpp>
Lumen::RHI::FRHIExpected Lumen::RHI::FRHIBufferDesc::Validate () const noexcept
{
    if ( Size == 0ULL or not IsValidRHIResourceUsage( Usage ) or not IsValidRHIResourceState( InitialState ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidResource( "Invalid buffer description" ) );
    }
    return {};
}
