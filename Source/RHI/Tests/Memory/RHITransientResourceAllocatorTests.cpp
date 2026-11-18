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

LUMEN_TEST( RHITransientResourceAllocator, DifferentMemoryClassesDoNotAlias )
{
    Tests::FNullDevice Device;
    const TSharedRef<IRHITransientResourceAllocator> Allocator = *Device.CreateTransientResourceAllocator();
    const FRHITransientDesc DeviceLocalRequest{
        .Requirements = { .Size                 = 64ULL,
                          .Alignment            = 16ULL,
                          .PreferredMemoryClass = TOptional<ERHIMemoryClass>( ERHIMemoryClass::DeviceLocal ) },
        .Lifetime     = { .Begin = 0ULL, .End = 1ULL } };
    const FRHITransientDesc UploadRequest{
        .Requirements = { .Size                 = 64ULL,
                          .Alignment            = 16ULL,
                          .PreferredMemoryClass = TOptional<ERHIMemoryClass>( ERHIMemoryClass::Upload ) },
        .Lifetime     = { .Begin = 1ULL, .End = 2ULL } };
    const FRHITransientBatch Batch{ { DeviceLocalRequest, UploadRequest } };

    Allocator->BeginBatch();
    const TExpected<TVector<FRHITransientAllocation>, FRHIError> Result = Allocator->Allocate( Batch );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result->size(), 2U );
    LUMEN_CHECK_NOT_EQ( ( *Result )[0].Allocation.Offset, ( *Result )[1].Allocation.Offset );
    LUMEN_CHECK_TRUE( ( *Result )[0].Allocation.MemoryClass == ERHIMemoryClass::DeviceLocal );
    LUMEN_CHECK_TRUE( ( *Result )[1].Allocation.MemoryClass == ERHIMemoryClass::Upload );
    Allocator->EndBatch();
}

LUMEN_TEST( RHITransientResourceAllocator, CompatibleRequirementsAliasDisjointLifetimes )
{
    Tests::FNullDevice Device;
    const TSharedRef<IRHITransientResourceAllocator> Allocator = *Device.CreateTransientResourceAllocator();
    const FRHITransientDesc First{ .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                   .Lifetime     = { .Begin = 0ULL, .End = 1ULL } };
    const FRHITransientDesc Second{ .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                    .Lifetime     = { .Begin = 1ULL, .End = 2ULL } };
    const FRHITransientBatch Batch{ { First, Second } };

    Allocator->BeginBatch();
    const TExpected<TVector<FRHITransientAllocation>, FRHIError> Result = Allocator->Allocate( Batch );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result->size(), 2U );
    LUMEN_CHECK_EQ( ( *Result )[0].Allocation.Offset, ( *Result )[1].Allocation.Offset );
    LUMEN_CHECK_EQ( ( *Result )[0].Allocation.Size, ( *Result )[1].Allocation.Size );
    LUMEN_CHECK_FALSE( ( *Result )[0].Lifetime.Overlaps( ( *Result )[1].Lifetime ) );
    Allocator->EndBatch();
}

LUMEN_TEST( RHITransientResourceAllocator, EqualMemoryClassesAliasDisjointLifetimes )
{
    Tests::FNullDevice Device;
    const TSharedRef<IRHITransientResourceAllocator> Allocator = *Device.CreateTransientResourceAllocator();
    const FRHITransientDesc First{
        .Requirements = { .Size                 = 64ULL,
                          .Alignment            = 16ULL,
                          .PreferredMemoryClass = TOptional<ERHIMemoryClass>( ERHIMemoryClass::Upload ) },
        .Lifetime     = { .Begin = 0ULL, .End = 1ULL } };
    const FRHITransientDesc Second{
        .Requirements = { .Size                 = 64ULL,
                          .Alignment            = 16ULL,
                          .PreferredMemoryClass = TOptional<ERHIMemoryClass>( ERHIMemoryClass::Upload ) },
        .Lifetime     = { .Begin = 1ULL, .End = 2ULL } };
    const FRHITransientBatch Batch{ { First, Second } };

    Allocator->BeginBatch();
    const TExpected<TVector<FRHITransientAllocation>, FRHIError> Result = Allocator->Allocate( Batch );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result->size(), 2U );
    LUMEN_CHECK_EQ( ( *Result )[0].Allocation.Offset, ( *Result )[1].Allocation.Offset );
    LUMEN_CHECK_TRUE( ( *Result )[1].Allocation.MemoryClass == ERHIMemoryClass::Upload );
    Allocator->EndBatch();
}

LUMEN_TEST( RHITransientResourceAllocator, LargerRequestDoesNotAliasSmallerRange )
{
    Tests::FNullDevice Device;
    const TSharedRef<IRHITransientResourceAllocator> Allocator = *Device.CreateTransientResourceAllocator();
    const FRHITransientDesc First{ .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                   .Lifetime     = { .Begin = 0ULL, .End = 1ULL } };
    const FRHITransientDesc Second{ .Requirements = { .Size = 128ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                    .Lifetime     = { .Begin = 1ULL, .End = 2ULL } };
    const FRHITransientBatch Batch{ { First, Second } };

    Allocator->BeginBatch();
    const TExpected<TVector<FRHITransientAllocation>, FRHIError> Result = Allocator->Allocate( Batch );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result->size(), 2U );
    LUMEN_CHECK_NOT_EQ( ( *Result )[0].Allocation.Offset, ( *Result )[1].Allocation.Offset );
    LUMEN_CHECK_TRUE( ( *Result )[1].Allocation.Size >= 128ULL );
    Allocator->EndBatch();
}

LUMEN_TEST( RHITransientResourceAllocator, StricterAlignmentDoesNotAliasLooserRange )
{
    Tests::FNullDevice Device;
    const TSharedRef<IRHITransientResourceAllocator> Allocator = *Device.CreateTransientResourceAllocator();
    const FRHITransientDesc First{ .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
                                   .Lifetime     = { .Begin = 0ULL, .End = 1ULL } };
    const FRHITransientDesc Second{ .Requirements = { .Size = 64ULL, .Alignment = 32ULL, .PreferredMemoryClass = {} },
                                    .Lifetime     = { .Begin = 1ULL, .End = 2ULL } };
    const FRHITransientBatch Batch{ { First, Second } };

    Allocator->BeginBatch();
    const TExpected<TVector<FRHITransientAllocation>, FRHIError> Result = Allocator->Allocate( Batch );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result->size(), 2U );
    LUMEN_CHECK_NOT_EQ( ( *Result )[0].Allocation.Offset, ( *Result )[1].Allocation.Offset );
    LUMEN_CHECK_TRUE( ( *Result )[1].Allocation.Alignment >= 32ULL );
    Allocator->EndBatch();
}

} // namespace Lumen
