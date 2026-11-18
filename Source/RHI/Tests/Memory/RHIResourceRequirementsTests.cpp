/**
 * @file RHIResourceRequirementsTests.cpp
 * @brief Tests requirement conversion.
 */

#include <RHI/Memory/RHIResourceRequirements.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIResourceRequirements, ConvertsToValidatedAllocation )
    {
        const FRHIResourceRequirements Requirements{
            .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = TOptional<ERHIMemoryClass>( ERHIMemoryClass::Upload ) };
        const TExpected<FRHIAllocationDesc, FRHIError> Desc = Requirements.ToAllocationDesc();

        LUMEN_CHECK_TRUE( Desc.HasValue() );
        LUMEN_CHECK_EQ( Desc->Size, 64ULL );
        LUMEN_CHECK_TRUE( Desc->MemoryClass == ERHIMemoryClass::Upload );
        LUMEN_CHECK_HAS_ERROR( FRHIResourceRequirements{}.ToAllocationDesc() );
    }

} // namespace RHI

} // namespace Lumen
