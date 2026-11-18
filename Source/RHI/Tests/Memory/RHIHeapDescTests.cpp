/**
 * @file RHIHeapDescTests.cpp
 * @brief Tests heap descriptor validation.
 */

#include <RHI/Memory/RHIHeapDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIHeapDesc, RequiresNonzeroSize )
    {
        LUMEN_CHECK_TRUE( FRHIHeapDesc{ .Size = 4096ULL }.Validate().HasValue() );
        LUMEN_CHECK_HAS_ERROR( FRHIHeapDesc{}.Validate() );
    }

} // namespace RHI

} // namespace Lumen
