/**
 * @file RHIAllocationDesc.cpp
 * @brief Implements allocation descriptor validation.
 */
#include <Memory/RHIAllocationDesc.hpp>
#include <Utilities/RHIAlignment.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIAllocationDesc::Validate () const noexcept
{
    if ( Size == 0ULL or not RHIIsPowerOfTwo( Alignment ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Allocation size and alignment are invalid" ) );
    }
    return {};
}
