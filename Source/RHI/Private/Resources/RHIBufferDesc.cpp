/**
 * @file RHIBufferDesc.cpp
 * @brief Implements buffer descriptor validation.
 */
#include <Resources/RHIBufferDesc.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIBufferDesc::Validate () const noexcept
{
    if ( Size == 0ULL or not IsValidRHIResourceUsage( Usage ) or not IsValidRHIResourceState( InitialState ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Resource, "Invalid buffer description" ) );
    }
    return {};
}
