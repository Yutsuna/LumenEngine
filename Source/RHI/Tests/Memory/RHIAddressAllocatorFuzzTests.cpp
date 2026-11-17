/**
 * @file RHIAddressAllocatorFuzzTests.cpp
 * @brief Fuzzes allocation against range, capacity, and ownership invariants.
 */

#include <Memory/RHILinearAllocator.hpp>
#include <Memory/RHIPoolAllocator.hpp>
#include <Memory/RHIRingAllocator.hpp>
#include <Memory/RHITlsfAllocator.hpp>
#include <Tests/UnitTest.hpp>
#include <Utilities/RHIAlignment.hpp>
#include <Utilities/RHITestRandom.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIAddressAllocatorFuzz, RandomOperationsNeverOverlapLiveRanges )
    {
        Tests::FRHITestRandom Random( 0x524849ULL );
        FRHITlsfAllocator Allocator( 4096ULL );
        TVector<FRHIAllocation> Active;

        for ( UInt32 Step = 0U; Step < 512U; ++Step )
        {
            if ( not Active.empty() and Random.Next( 3ULL ) == 0ULL )
            {
                const USize Index = static_cast<USize>( Random.Next( Active.size() ) );
                LUMEN_CHECK_TRUE( Allocator.Free( Active[Index] ).HasValue() );
                LUMEN_CHECK_HAS_ERROR( Allocator.Free( Active[Index] ) );
                Active.erase( Active.begin() + static_cast<ISize>( Index ) );
                continue;
            }
            const UInt64 Size = Random.Next( 64ULL ) + 1ULL;
            auto Allocation   = Allocator.Allocate( { .Size = Size, .Alignment = 16ULL } );
            if ( not Allocation )
            {
                continue;
            }
            LUMEN_CHECK_EQ( Allocation->Offset % 16ULL, 0ULL );
            for ( const FRHIAllocation &Other : Active )
            {
                LUMEN_CHECK_FALSE( RHIRangesOverlap( Allocation->Offset, Allocation->Size, Other.Offset, Other.Size ) );
            }
            Active.push_back( *Allocation );
        }
        for ( const FRHIAllocation &Allocation : Active )
        {
            LUMEN_CHECK_TRUE( Allocator.Free( Allocation ).HasValue() );
        }
        LUMEN_CHECK_EQ( Allocator.GetStats().Used, 0ULL );
        LUMEN_CHECK_HAS_ERROR( Allocator.Free( { .Id = FRHIAllocationId{ 0xFFFFFFFFFFFFFFFFULL }, .Size = 1ULL } ) );
    }

    LUMEN_TEST( RHIAddressAllocatorFuzz, RingAndPoolRejectInvalidOwnershipAndDoubleFree )
    {
        Tests::FRHITestRandom Random( 0x414C4C4F4352554EULL );
        FRHIRingAllocator Ring( 4096ULL );
        FRHIPoolAllocator Pool( 64ULL, 64U );
        TVector<FRHIAllocation> RingActive;
        TVector<FRHIAllocation> PoolActive;

        for ( UInt32 Step = 0U; Step < 512U; ++Step )
        {
            if ( not RingActive.empty() and Random.Next( 2ULL ) == 0ULL )
            {
                const USize Index = static_cast<USize>( Random.Next( RingActive.size() ) );
                LUMEN_CHECK_TRUE( Ring.Free( RingActive[Index] ).HasValue() );
                LUMEN_CHECK_HAS_ERROR( Ring.Free( RingActive[Index] ) );
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

            if ( not PoolActive.empty() and Random.Next( 2ULL ) == 0ULL )
            {
                const USize Index = static_cast<USize>( Random.Next( PoolActive.size() ) );
                LUMEN_CHECK_TRUE( Pool.Free( PoolActive[Index] ).HasValue() );
                LUMEN_CHECK_HAS_ERROR( Pool.Free( PoolActive[Index] ) );
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
        LUMEN_CHECK_HAS_ERROR( Ring.Free( { .Id = FRHIAllocationId{ 0xFFFFFFFFFFFFFFFFULL }, .Size = 1ULL } ) );
        LUMEN_CHECK_HAS_ERROR( Pool.Free( { .Id = FRHIAllocationId{ 0xFFFFFFFFFFFFFFFFULL }, .Size = 64ULL } ) );
    }

    LUMEN_TEST( RHIAddressAllocatorFuzz, LinearRandomPassesRespectAlignmentAndCapacity )
    {
        Tests::FRHITestRandom Random( 0x4C494E454152ULL );
        FRHILinearAllocator Allocator( 1024ULL );
        for ( UInt32 Pass = 0U; Pass < 64U; ++Pass )
        {
            for ( UInt32 Index = 0U; Index < 32U; ++Index )
            {
                const UInt64 Size      = Random.Next( 96ULL ) + 1ULL;
                const UInt64 Alignment = 1ULL << Random.Next( 6ULL );
                auto Allocation        = Allocator.Allocate( { .Size = Size, .Alignment = Alignment } );
                if ( Allocation )
                {
                    LUMEN_CHECK_EQ( Allocation->Offset % Alignment, 0ULL );
                    LUMEN_CHECK_TRUE( Allocation->Offset + Allocation->Size <= 1024ULL );
                    LUMEN_CHECK_TRUE( Allocator.GetStats().Used <= 1024ULL );
                }
            }
            Allocator.Reset();
            LUMEN_CHECK_EQ( Allocator.GetStats().Used, 0ULL );
        }
        LUMEN_CHECK_HAS_ERROR( Allocator.Free( {} ) );
    }

} // namespace
} // namespace Lumen
