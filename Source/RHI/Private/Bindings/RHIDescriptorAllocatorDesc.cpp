/**
 * @file RHIDescriptorAllocatorDesc.cpp
 * @brief Implements descriptor allocator policy validation.
 */

#include <Bindings/RHIDescriptorAllocatorDesc.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIDescriptorAllocatorDesc::Validate () const noexcept
{
    if ( InitialCapacity == 0U or MaximumCapacity < InitialCapacity )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Invalid descriptor capacities" ) );
    }
    return {};
}
