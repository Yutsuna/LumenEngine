/**
 * @file RHIMemoryStressTests.cpp
 * @brief Stresses interleaved allocation, recycling, and transient placement.
 */

#include <Device/NullDevice.hpp>
#include <Memory/IRHIReadbackAllocator.hpp>
#include <Memory/IRHITransientResourceAllocator.hpp>
#include <Memory/IRHIUploadAllocator.hpp>
#include <Memory/RHILinearAllocator.hpp>
#include <Memory/RHIPoolAllocator.hpp>
#include <Memory/RHIRingAllocator.hpp>
#include <Memory/RHITlsfAllocator.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>
#include <Utilities/RHIAlignment.hpp>
#include <Utilities/RHITestRandom.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIMemoryStress, RepeatedAllocationAndCoalescingStayBalanced )
    {
        FRHITlsfAllocator Allocator( 65536ULL );

        for ( UInt32 Index = 0U; Index < 512U; ++Index )
        {
            auto Allocation = Allocator.Allocate( { .Size = 64ULL, .Alignment = 16ULL } );
            LUMEN_CHECK_TRUE( Allocation.HasValue() );
            LUMEN_CHECK_TRUE( Allocator.Free( *Allocation ).HasValue() );
        }

        LUMEN_CHECK_EQ( Allocator.GetStats().Used, 0ULL );
    }

    LUMEN_TEST( RHIMemoryStress, InterleavedRingAndPoolOperationsRecycleWithoutOverlap )
    {
        Tests::FRHITestRandom Random( 0x52494E47504F4F4CULL );
        FRHIRingAllocator Ring( 4096ULL );
        FRHIPoolAllocator Pool( 64ULL, 64U );
        TVector<FRHIAllocation> RingActive;
        TVector<FRHIAllocation> PoolActive;

        for ( UInt32 Step = 0U; Step < 512U; ++Step )
        {
            if ( not RingActive.empty() and Random.Next( 3ULL ) == 0ULL )
            {
                const USize Index = static_cast<USize>( Random.Next( RingActive.size() ) );
                LUMEN_CHECK_TRUE( Ring.Free( RingActive[Index] ).HasValue() );
                RingActive.erase( RingActive.begin() + static_cast<ISize>( Index ) );
            }
            else
            {
                auto Allocation = Ring.Allocate( { .Size = Random.Next( 96ULL ) + 1ULL, .Alignment = 16ULL } );
                if ( Allocation )
                {
                    for ( const FRHIAllocation &Other : RingActive )
                    {
                        LUMEN_CHECK_FALSE( RHIRangesOverlap( Allocation->Offset, Allocation->Size, Other.Offset, Other.Size ) );
                    }
                    RingActive.push_back( *Allocation );
                }
            }

            if ( not PoolActive.empty() and Random.Next( 3ULL ) == 0ULL )
            {
                const USize Index = static_cast<USize>( Random.Next( PoolActive.size() ) );
                LUMEN_CHECK_TRUE( Pool.Free( PoolActive[Index] ).HasValue() );
                PoolActive.erase( PoolActive.begin() + static_cast<ISize>( Index ) );
            }
            else
            {
                auto Allocation = Pool.Allocate( { .Size = Random.Next( 64ULL ) + 1ULL, .Alignment = 16ULL } );
                if ( Allocation )
                {
                    for ( const FRHIAllocation &Other : PoolActive )
                    {
                        LUMEN_CHECK_FALSE( RHIRangesOverlap( Allocation->Offset, Allocation->Size, Other.Offset, Other.Size ) );
                    }
                    PoolActive.push_back( *Allocation );
                }
            }
        }

        for ( const FRHIAllocation &Allocation : RingActive )
        {
            LUMEN_CHECK_TRUE( Ring.Free( Allocation ).HasValue() );
        }
        for ( const FRHIAllocation &Allocation : PoolActive )
        {
            LUMEN_CHECK_TRUE( Pool.Free( Allocation ).HasValue() );
        }
        LUMEN_CHECK_EQ( Ring.GetStats().Used, 0ULL );
        LUMEN_CHECK_EQ( Pool.GetStats().Used, 0ULL );

        auto RingAllocation = Ring.Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        auto PoolAllocation = Pool.Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
        LUMEN_CHECK_TRUE( RingAllocation.HasValue() );
        LUMEN_CHECK_TRUE( PoolAllocation.HasValue() );
        LUMEN_CHECK_TRUE( Ring.Free( *RingAllocation ).HasValue() );
        LUMEN_CHECK_TRUE( Pool.Free( *PoolAllocation ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( Ring.Free( *RingAllocation ) );
        LUMEN_CHECK_HAS_ERROR( Pool.Free( *PoolAllocation ) );
    }

    LUMEN_TEST( RHIMemoryStress, LinearResetAndConcurrentTlsfRemainBalanced )
    {
        FRHILinearAllocator Linear( 256ULL );
        for ( UInt32 Pass = 0U; Pass < 128U; ++Pass )
        {
            Bool bExhausted = false;
            for ( UInt32 Index = 0U; Index < 16U; ++Index )
            {
                auto Allocation = Linear.Allocate( { .Size = 32ULL, .Alignment = 16ULL } );
                bExhausted      = bExhausted or not Allocation;
            }
            LUMEN_CHECK_TRUE( bExhausted );
            Linear.Reset();
            LUMEN_CHECK_EQ( Linear.GetStats().Used, 0ULL );
        }

        FRHITlsfAllocator Allocator( 65536ULL );
        TAtomic<UInt32> Failures{ 0U };
        TVector<FThread> Workers;
        for ( UInt32 Worker = 0U; Worker < 4U; ++Worker )
        {
            Workers.emplace_back( [&Allocator, &Failures, Worker]
            {
                Tests::FRHITestRandom Random( 0x544C5346ULL + Worker );
                TVector<FRHIAllocation> Active;
                for ( UInt32 Step = 0U; Step < 256U; ++Step )
                {
                    if ( not Active.empty() and Random.Next( 3ULL ) == 0ULL )
                    {
                        const USize Index = static_cast<USize>( Random.Next( Active.size() ) );
                        if ( not Allocator.Free( Active[Index] ) )
                        {
                            Failures.fetch_add( 1U );
                        }
                        Active.erase( Active.begin() + static_cast<ISize>( Index ) );
                    }
                    else
                    {
                        auto Allocation = Allocator.Allocate( { .Size = Random.Next( 96ULL ) + 1ULL, .Alignment = 16ULL } );
                        if ( Allocation )
                        {
                            for ( const FRHIAllocation &Other : Active )
                            {
                                if ( RHIRangesOverlap( Allocation->Offset, Allocation->Size, Other.Offset, Other.Size ) )
                                {
                                    Failures.fetch_add( 1U );
                                }
                            }
                            Active.push_back( *Allocation );
                        }
                    }
                }
                for ( const FRHIAllocation &Allocation : Active )
                {
                    if ( not Allocator.Free( Allocation ) )
                    {
                        Failures.fetch_add( 1U );
                    }
                }
            } );
        }
        for ( auto &Worker : Workers )
        {
            Worker.Join();
        }
        LUMEN_CHECK_EQ( Failures.load(), 0U );
        LUMEN_CHECK_EQ( Allocator.GetStats().Used, 0ULL );
    }

    LUMEN_TEST( RHIMemoryStress, StagingRingsRecycleInterleavedRanges )
    {
        Tests::FNullDevice Device;
        auto Upload   = *Device.CreateUploadAllocator( 2048ULL );
        auto Readback = *Device.CreateReadbackAllocator( 2048ULL );
        Tests::FRHITestRandom Random( 0x53544147494E47ULL );
        TVector<FRHIAllocation> UploadActive;
        TVector<FRHIAllocation> ReadbackActive;

        for ( UInt32 Step = 0U; Step < 256U; ++Step )
        {
            auto UploadAllocation = Upload->Allocate( { .Size = Random.Next( 64ULL ) + 1ULL, .Alignment = 16ULL } );
            if ( UploadAllocation )
            {
                UploadAllocation->MappedData[0] = static_cast<Byte>( Step );
                UploadActive.push_back( UploadAllocation->Allocation );
            }
            auto ReadbackAllocation = Readback->Allocate( { .Size = Random.Next( 64ULL ) + 1ULL, .Alignment = 16ULL } );
            if ( ReadbackAllocation )
            {
                auto Mapped = Readback->Map( *ReadbackAllocation );
                if ( not Mapped or Mapped->MappedData.size() != ReadbackAllocation->Size )
                {
                    LUMEN_CHECK_TRUE( false );
                }
                ReadbackActive.push_back( *ReadbackAllocation );
            }
            if ( not UploadActive.empty() )
            {
                const USize Index = static_cast<USize>( Random.Next( UploadActive.size() ) );
                LUMEN_CHECK_TRUE( Upload->Free( UploadActive[Index] ).HasValue() );
                UploadActive.erase( UploadActive.begin() + static_cast<ISize>( Index ) );
            }
            if ( not ReadbackActive.empty() )
            {
                const USize Index = static_cast<USize>( Random.Next( ReadbackActive.size() ) );
                LUMEN_CHECK_TRUE( Readback->Free( ReadbackActive[Index] ).HasValue() );
                ReadbackActive.erase( ReadbackActive.begin() + static_cast<ISize>( Index ) );
            }
        }
        for ( const FRHIAllocation &Allocation : UploadActive )
        {
            LUMEN_CHECK_TRUE( Upload->Free( Allocation ).HasValue() );
        }
        for ( const FRHIAllocation &Allocation : ReadbackActive )
        {
            LUMEN_CHECK_TRUE( Readback->Free( Allocation ).HasValue() );
        }
    }

    LUMEN_TEST( RHIMemoryStress, TransientBatchesNeverAliasOverlappingLifetimes )
    {
        Tests::FNullDevice Device;
        auto Allocator = *Device.CreateTransientResourceAllocator();
        Tests::FRHITestRandom Random( 0x5452414E5349454EULL );
        for ( UInt32 Pass = 0U; Pass < 128U; ++Pass )
        {
            FRHITransientBatch Batch;
            for ( UInt32 Index = 0U; Index < 8U; ++Index )
            {
                FRHITransientDesc Request;
                Request.Requirements.Size      = Random.Next( 2ULL ) == 0ULL ? 32ULL : 64ULL;
                Request.Requirements.Alignment = 16ULL;
                Request.Lifetime.Begin         = Random.Next( 4ULL );
                Request.Lifetime.End           = Request.Lifetime.Begin + Random.Next( 3ULL ) + 1ULL;
                Batch.Requests.push_back( Request );
            }
            Allocator->BeginBatch();
            auto Allocations = Allocator->Allocate( Batch );
            Allocator->EndBatch();
            LUMEN_CHECK_TRUE( Allocations.HasValue() );
            for ( USize First = 0U; First < Allocations->size(); ++First )
            {
                for ( USize Second = First + 1U; Second < Allocations->size(); ++Second )
                {
                    const auto &Left  = ( *Allocations )[First];
                    const auto &Right = ( *Allocations )[Second];
                    if ( Left.Lifetime.Overlaps( Right.Lifetime ) )
                    {
                        LUMEN_CHECK_FALSE( RHIRangesOverlap( Left.Allocation.Offset, Left.Allocation.Size,
                                                             Right.Allocation.Offset, Right.Allocation.Size ) );
                    }
                }
            }
        }
    }

} // namespace
} // namespace Lumen
