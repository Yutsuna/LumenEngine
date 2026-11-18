/**
 * @file RHIAllocationDesc.cpp
 * @brief Implements allocation descriptor validation.
 */

#include <RHI/Memory/RHIAllocationDesc.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIAllocationDesc::Validate () const noexcept
{
    if ( Size == 0ULL or not RHIIsPowerOfTwo( Alignment ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Allocation size and alignment are invalid" ) );
    }
    return {};
}
