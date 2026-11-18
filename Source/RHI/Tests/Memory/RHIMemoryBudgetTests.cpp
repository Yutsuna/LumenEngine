/**
 * @file RHIMemoryBudgetTests.cpp
 * @brief Tests atomic budget admission.
 */

#include <Memory/RHIMemoryBudget.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIMemoryBudget, NeverExceedsCapacity )
{
    FRHIMemoryBudget Budget( 64ULL );
    LUMEN_CHECK_TRUE( Budget.TryReserve( 32ULL ) );
    LUMEN_CHECK_TRUE( Budget.TryReserve( 32ULL ) );
    LUMEN_CHECK_FALSE( Budget.TryReserve( 1ULL ) );
    Budget.Release( 32ULL );
    LUMEN_CHECK_EQ( Budget.GetUsed(), 32ULL );
}

} // namespace Lumen
