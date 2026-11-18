/**
 * @file RHIMemoryBudgetTests.cpp
 * @brief Tests atomic budget admission.
 */

#include <RHI/Memory/RHIMemoryBudget.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
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

} // namespace RHI

} // namespace Lumen
