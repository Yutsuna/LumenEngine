/**
 * @file RHIValidationTransientResourceAllocatorTests.cpp
 * @brief Tests transient batch scopes, aliasing, and lifetime expiration.
 */
#include "Validation/RHIValidationTestUtils.hpp"
#include "Validation/RHIValidationTransientResourceAllocator.hpp"
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{
    FRHITransientDesc MakeRequest ( const UInt64 InBegin, const UInt64 InEnd ) noexcept
    {
        FRHITransientDesc Request;
        Request.Requirements.Size      = 64ULL;
        Request.Requirements.Alignment = 16ULL;
        Request.Lifetime               = { InBegin, InEnd };
        return Request;
    }

    LUMEN_TEST( RHIValidationTransientResourceAllocator, RequiresActiveBatchAndLiveFrame )
    {
        Tests::FRecordingDebugSink Sink;
        TSharedRef<IRHIDevice> Device                        = Tests::Validation::MakeDevice( Sink );
        TSharedRef<IRHITransientResourceAllocator> Allocator = *Device->CreateTransientResourceAllocator();
        auto *Validation = dynamic_cast<FRHIValidationTransientResourceAllocator *>( Allocator.Get() );
        LUMEN_CHECK_TRUE( Validation != nullptr );
        FRHITransientBatch Batch;
        Batch.Requests.push_back( MakeRequest( 0ULL, 1ULL ) );
        LUMEN_CHECK_EQ( Allocator->Allocate( Batch ).Error().Code, ERHIErrorCode::ValidationFailed );
        Allocator->BeginBatch();
        TVector<FRHITransientAllocation> Allocations = *Allocator->Allocate( Batch );
        LUMEN_CHECK_EQ( Allocations.size(), 1U );
        LUMEN_CHECK_TRUE( Validation->ValidateResourceUse( Allocations[0], 0ULL ).HasValue() );
        LUMEN_CHECK_EQ( Validation->ValidateResourceUse( Allocations[0], 1ULL ).Error().Code, ERHIErrorCode::ValidationFailed );
        Allocator->EndBatch();
        LUMEN_CHECK_EQ( Validation->ValidateResourceUse( Allocations[0], 0ULL ).Error().Code, ERHIErrorCode::ValidationFailed );
    }

    LUMEN_TEST( RHIValidationTransientResourceAllocator, PreservesDisjointLifetimesAndSeparatesOverlap )
    {
        Tests::FRecordingDebugSink Sink;
        TSharedRef<IRHIDevice> Device                        = Tests::Validation::MakeDevice( Sink );
        TSharedRef<IRHITransientResourceAllocator> Allocator = *Device->CreateTransientResourceAllocator();
        FRHITransientBatch Batch;
        Batch.Requests.push_back( MakeRequest( 0ULL, 1ULL ) );
        Batch.Requests.push_back( MakeRequest( 1ULL, 2ULL ) );
        Allocator->BeginBatch();
        TVector<FRHITransientAllocation> Disjoint = *Allocator->Allocate( Batch );
        LUMEN_CHECK_EQ( Disjoint[0].Allocation.Offset, Disjoint[1].Allocation.Offset );
        Allocator->EndBatch();
        Batch.Requests[1].Lifetime = { 0ULL, 2ULL };
        Allocator->BeginBatch();
        TVector<FRHITransientAllocation> Overlapping = *Allocator->Allocate( Batch );
        LUMEN_CHECK_NOT_EQ( Overlapping[0].Allocation.Offset, Overlapping[1].Allocation.Offset );
        Allocator->EndBatch();
    }
} // namespace
} // namespace Lumen
