/**
 * @file RHIDescriptorAllocatorDesc.cpp
 * @brief Implements descriptor allocator policy validation.
 */

#include <RHI/Bindings/RHIDescriptorAllocatorDesc.hpp>

/**
 * Public
 */

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIDescriptorAllocatorDesc::Validate () const noexcept
{
    if ( InitialCapacity == 0U or MaximumCapacity < InitialCapacity )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Invalid descriptor capacities" ) );
    }
    return {};
}
