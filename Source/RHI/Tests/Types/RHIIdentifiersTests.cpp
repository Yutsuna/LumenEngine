/**
 * @file RHIIdentifiersTests.cpp
 * @brief Tests typed identifiers and generation.
 */

#include <Tests/UnitTest.hpp>
#include <Types/RHIIdentifiers.hpp>

namespace Lumen
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

} // namespace Lumen
