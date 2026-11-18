/**
 * @file RHIAllocationDescTests.cpp
 * @brief Tests allocation descriptor validation.
 */

#include <RHI/Memory/RHIAllocationDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIAllocationDesc, ValidatesSizeAndAlignment )
    {
        LUMEN_CHECK_TRUE( ( FRHIAllocationDesc{ 32ULL, 16ULL }.Validate().HasValue() ) );
        LUMEN_CHECK_HAS_ERROR( FRHIAllocationDesc{}.Validate() );
        LUMEN_CHECK_HAS_ERROR( ( FRHIAllocationDesc{ 32ULL, 3ULL }.Validate() ) );
    }

} // namespace RHI

} // namespace Lumen
