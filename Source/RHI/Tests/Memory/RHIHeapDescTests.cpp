/**
 * @file RHIHeapDescTests.cpp
 * @brief Tests heap descriptor validation.
 */

#include <Memory/RHIHeapDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIHeapDesc, RequiresNonzeroSize )
{
    LUMEN_CHECK_TRUE( FRHIHeapDesc{ .Size = 4096ULL }.Validate().HasValue() );
    LUMEN_CHECK_HAS_ERROR( FRHIHeapDesc{}.Validate() );
}

} // namespace Lumen
