/**
 * @file RHIResourceRequirements.cpp
 * @brief Implements conversion of resource requirements.
 */

#include <RHI/Memory/RHIResourceRequirements.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::RHI::FRHIAllocationDesc, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIResourceRequirements::ToAllocationDesc () const noexcept
{
    const FRHIAllocationDesc Desc{
        .Size = Size, .Alignment = Alignment, .MemoryClass = PreferredMemoryClass.ValueOr( ERHIMemoryClass::DeviceLocal ) };

    LUMEN_EXPECT( _, Desc.Validate() );
    return Desc;
}
