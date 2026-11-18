/**
 * @file RHIIdentifiersTests.cpp
 * @brief Tests typed identifiers and generation.
 */

#include <RHI/Types/RHIIdentifiers.hpp>
#include <Tests/UnitTest.hpp>
#include <limits>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIIdentifiers, GeneratesUniqueMonotonicValues )
    {
        FRHIIdentifierGenerator Generator;
        const FRHIObjectId First  = Generator.Next<FRHIObjectIdTag>();
        const FRHIObjectId Second = Generator.Next<FRHIObjectIdTag>();
        LUMEN_CHECK_TRUE( First.IsValid() and Second.IsValid() );
        LUMEN_CHECK_TRUE( First < Second );
        LUMEN_CHECK_FALSE( FRHIObjectId{}.IsValid() );
        LUMEN_CHECK_EQ( First.GetValue(), 1ULL );
    }

    LUMEN_TEST( RHIIdentifiers, StaysExhaustedWithoutReissuingIds )
    {
        constexpr UInt64 MaxValue = std::numeric_limits<UInt64>::max();
        FRHIIdentifierGenerator Generator( MaxValue - 2ULL );

        const FRHIObjectId FirstValid  = Generator.Next<FRHIObjectIdTag>();
        const FRHIObjectId SecondValid = Generator.Next<FRHIObjectIdTag>();
        const FRHIObjectId LastValid   = Generator.Next<FRHIObjectIdTag>();

        LUMEN_CHECK_TRUE( FirstValid.IsValid() );
        LUMEN_CHECK_TRUE( SecondValid.IsValid() );
        LUMEN_CHECK_TRUE( LastValid.IsValid() );
        LUMEN_CHECK_EQ( FirstValid.GetValue(), MaxValue - 2ULL );
        LUMEN_CHECK_EQ( SecondValid.GetValue(), MaxValue - 1ULL );
        LUMEN_CHECK_EQ( LastValid.GetValue(), MaxValue );
        LUMEN_CHECK_TRUE( FirstValid < SecondValid );
        LUMEN_CHECK_TRUE( SecondValid < LastValid );
        LUMEN_CHECK_NOT_EQ( FirstValid.GetValue(), SecondValid.GetValue() );
        LUMEN_CHECK_NOT_EQ( SecondValid.GetValue(), LastValid.GetValue() );
        LUMEN_CHECK_NOT_EQ( FirstValid.GetValue(), LastValid.GetValue() );

        const FRHIObjectId AfterExhaustion = Generator.Next<FRHIObjectIdTag>();
        LUMEN_CHECK_FALSE( AfterExhaustion.IsValid() );
        LUMEN_CHECK_EQ( AfterExhaustion.GetValue(), 0ULL );

        /** NOTE: Repeated calls must stay invalid; a wraparound would reissue low identifiers. */
        for ( UInt32 Attempt = 0U; Attempt < 8U; ++Attempt )
        {
            LUMEN_CHECK_FALSE( Generator.Next<FRHIObjectIdTag>().IsValid() );
        }
    }

} // namespace RHI

} // namespace Lumen
