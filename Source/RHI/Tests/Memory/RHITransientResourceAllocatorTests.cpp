/**
 * @file RHITransientResourceAllocatorTests.cpp
 * @brief Tests transient batch scopes and allocation.
 */

#include <Device/NullDevice.hpp>
#include <Memory/IRHITransientResourceAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHITransientResourceAllocator, AllocatesOnlyInsideBatch )
{
    Tests::FNullDevice Device;
    const TSharedRef<IRHITransientResourceAllocator> Allocator = *Device.CreateTransientResourceAllocator();
    const FRHITransientDesc Request{ .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                     .Lifetime     = { .Begin = 0ULL, .End = 1ULL } };
    const FRHITransientBatch Batch{ { Request, Request } };

    LUMEN_CHECK_HAS_ERROR( Allocator->Allocate( Batch ) );
    Allocator->BeginBatch();

    const TExpected<TVector<FRHITransientAllocation>, FRHIError> Result = Allocator->Allocate( Batch );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result->size(), 2U );

    Allocator->EndBatch();
}

} // namespace Lumen
