/**
 * @file RHIResourceRequirements.cpp
 * @brief Implements conversion of resource requirements.
 */

#include <RHI/Memory/RHIResourceRequirements.hpp>

Lumen::TExpected<Lumen::RHI::FRHIAllocationDesc, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIResourceRequirements::ToAllocationDesc () const noexcept
{
    const FRHIAllocationDesc Desc{
        .Size = Size, .Alignment = Alignment, .MemoryClass = PreferredMemoryClass.ValueOr( ERHIMemoryClass::DeviceLocal ) };

    if ( const TExpected<void, FRHIError> Valid = Desc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    return Desc;
}
