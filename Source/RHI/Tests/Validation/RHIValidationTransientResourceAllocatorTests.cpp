/**
 * @file RHIValidationTransientResourceAllocatorTests.cpp
 * @brief Tests transient batch scopes and allocation aliasing through the public decorator path.
 */
#include "Validation/RHIValidationReporter.hpp"
#include "Validation/RHIValidationTestUtils.hpp"
#include <RHI/Memory/IRHITransientResourceAllocator.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
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

        LUMEN_TEST( RHIValidationTransientResourceAllocator, RejectsAllocateOutsideActiveBatch )
        {
            Tests::FRecordingDebugSink Sink;
            TSharedRef<IRHIDevice> Device                        = Tests::Validation::MakeDevice( Sink );
            TSharedRef<IRHITransientResourceAllocator> Allocator = *Device->CreateTransientResourceAllocator();
            FRHITransientBatch Batch;
            Batch.Requests.push_back( MakeRequest( 0ULL, 1ULL ) );
            LUMEN_CHECK_EQ( Allocator->Allocate( Batch ).Error().Code, ERHIErrorCode::ValidationFailed );
            LUMEN_CHECK_TRUE( Sink.ContainsCode( static_cast<UInt32>( ERHIValidationMessageCode::InvalidTransientBatch ) ) );
            Allocator->BeginBatch();
            TVector<FRHITransientAllocation> Allocations = *Allocator->Allocate( Batch );
            LUMEN_CHECK_EQ( Allocations.size(), 1U );
            Allocator->EndBatch();
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
} // namespace RHI

} // namespace Lumen
