/**
 * @file RHITransientBatchTests.cpp
 * @brief Tests transient batch validation.
 */

#include <RHI/Memory/RHITransientBatch.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHITransientBatch, ValidatesEachRequest )
    {
        const FRHITransientDesc Valid{ .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                       .Lifetime     = { 0ULL, 1ULL } };
        const FRHITransientBatch Batch{ { Valid } };
        LUMEN_CHECK_TRUE( Batch.Validate().HasValue() );
        const FRHITransientBatch Invalid{ { FRHITransientDesc{} } };
        LUMEN_CHECK_HAS_ERROR( Invalid.Validate() );
    }

} // namespace RHI

} // namespace Lumen
