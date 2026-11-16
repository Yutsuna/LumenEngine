/**
 * @file RHIHashTests.cpp
 * @brief Tests deterministic RHI content hashing.
 */

#include <Tests/UnitTest.hpp>
#include <Types/RHIHash.hpp>
#include <Types/RHIIdentifiers.hpp>

namespace Lumen
{

LUMEN_TEST( RHIHash, HashesContentAndTypedIdentifiers )
{
    constexpr UInt32 Value = 0x12345678U;
    constexpr UInt32 Other = 0x12345679U;
    LUMEN_CHECK_EQ( RHIHashValue( Value ), RHIHashValue( Value ) );
    LUMEN_CHECK_NOT_EQ( RHIHashValue( Value ), RHIHashValue( Other ) );
    LUMEN_CHECK_EQ( TRHIValueHasher<FRHIObjectId>{}( FRHIObjectId{ 7ULL } ),
                    TRHIValueHasher<FRHIObjectId>{}( FRHIObjectId{ 7ULL } ) );
    LUMEN_CHECK_NOT_EQ( RHIHashCombine( 1ULL, 2ULL ), RHIHashCombine( 2ULL, 1ULL ) );
}

} // namespace Lumen
