/**
 * @file RHIAddressAllocatorFuzzTests.cpp
 * @brief Fuzzes variable allocation against range invariants.
 * */

#include <Memory/RHITlsfAllocator.hpp>
#include <Tests/UnitTest.hpp>
#include <Utilities/RHIAlignment.hpp>
#include <Utilities/RHITestRandom.hpp>

namespace Lumen
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
            Active.erase( Active.begin() + static_cast<ISize>( Index ) );
            continue;
        }
        const UInt64 Size = Random.Next( 64ULL ) + 1ULL;
        auto Allocation   = Allocator.Allocate( { .Size = Size, .Alignment = 16ULL } );
        if ( not Allocation )
        {
            continue;
        }
        for ( const FRHIAllocation &Other : Active )
        {
            LUMEN_CHECK_FALSE( RHIRangesOverlap( Allocation->Offset, Allocation->Size, Other.Offset, Other.Size ) );
        }
        Active.push_back( *Allocation );
    }
}

} // namespace Lumen
