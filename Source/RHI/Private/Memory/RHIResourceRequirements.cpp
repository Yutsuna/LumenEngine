/**
 * @file RHIResourceRequirements.cpp
 * @brief Implements conversion of resource requirements.
 */
#include <Memory/RHIResourceRequirements.hpp>
Lumen::TExpected<Lumen::FRHIAllocationDesc, Lumen::FRHIError> Lumen::FRHIResourceRequirements::ToAllocationDesc () const noexcept
{
    const FRHIAllocationDesc Desc{ Size, Alignment, PreferredMemoryClass.ValueOr( ERHIMemoryClass::DeviceLocal ) };
    if ( const auto Valid = Desc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    return Desc;
}
