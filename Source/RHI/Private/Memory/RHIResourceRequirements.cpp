/**
 * @file RHIResourceRequirements.cpp
 * @brief Implements conversion of resource requirements.
 */

#include <Memory/RHIResourceRequirements.hpp>

Lumen::TExpected<Lumen::FRHIAllocationDesc, Lumen::FRHIError> Lumen::FRHIResourceRequirements::ToAllocationDesc () const noexcept
{
    const FRHIAllocationDesc Desc{
        .Size = Size, .Alignment = Alignment, .MemoryClass = PreferredMemoryClass.ValueOr( ERHIMemoryClass::DeviceLocal ) };

    if ( const TExpected<void, FRHIError> Valid = Desc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    return Desc;
}
