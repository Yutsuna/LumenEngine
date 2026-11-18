/**
 * @file RHIAllocationTests.cpp
 * @brief Tests allocation validity and containment.
 */

#include <Memory/RHIAllocation.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIAllocation, ContainsOnlyInteriorRanges )
{
    const FRHIAllocation Allocation{ FRHIAllocationId{ 1ULL }, 16ULL, 32ULL };
    LUMEN_CHECK_TRUE( Allocation.IsValid() );
    LUMEN_CHECK_TRUE( Allocation.Contains( 16ULL, 32ULL ) );
    LUMEN_CHECK_TRUE( Allocation.Contains( 24ULL, 8ULL ) );
    LUMEN_CHECK_FALSE( Allocation.Contains( 0ULL, 4ULL ) );
    LUMEN_CHECK_FALSE( Allocation.Contains( 40ULL, 16ULL ) );
}

} // namespace Lumen
