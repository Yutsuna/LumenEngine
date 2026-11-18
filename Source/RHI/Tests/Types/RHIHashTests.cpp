/**
 * @file RHIHashTests.cpp
 * @brief Tests deterministic RHI content hashing.
 */

#include <RHI/Types/RHIHash.hpp>
#include <RHI/Types/RHIIdentifiers.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
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

} // namespace RHI

} // namespace Lumen
