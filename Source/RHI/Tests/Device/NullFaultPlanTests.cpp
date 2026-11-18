/**
 * @file NullFaultPlanTests.cpp
 * @brief Tests deterministic occurrence-based faults.
 */

#include <Device/NullFaultPlan.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( NullFaultPlan, FailsOnlyTheRequestedOccurrence )
    {
        Tests::FNullFaultPlan Plan;
        Plan.AddFailure( "CreateBuffer", 2U, MakeRHIOutOfMemory( ERHIErrorCondition::Memory, "planned" ) );
        LUMEN_CHECK_FALSE( Plan.ShouldFail( "CreateBuffer" ).HasValue() );
        const TOptional<FRHIError> Error = Plan.ShouldFail( "CreateBuffer" );
        LUMEN_CHECK_TRUE( Error.HasValue() );
        LUMEN_CHECK_EQ( static_cast<UInt16>( Error->Code ), static_cast<UInt16>( ERHIErrorCode::OutOfMemory ) );
        LUMEN_CHECK_FALSE( Plan.ShouldFail( "CreateBuffer" ).HasValue() );
        LUMEN_CHECK_FALSE( Plan.ShouldFail( "CreateTexture" ).HasValue() );
    }

} // namespace RHI

} // namespace Lumen
