/**
 * @file RHIDescriptorAllocatorBaseTests.cpp
 * @brief Tests common descriptor range allocation and pool lifetime.
 */

#include "Bindings/NullBindings.hpp"
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{
    class FTrackingDescriptorBackend final : public IRHIDescriptorPoolBackend
    {
    public:

        TExpected<UInt64, FRHIError> CreatePool ( const UInt32 InCapacity ) noexcept override
        {
            if ( InCapacity == 0U )
            {
                return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Zero capacity" ) );
            }
            ++CreateCount;
            return 42ULL;
        }
        void ReleasePool ( const UInt64 InPool ) noexcept override
        {
            LUMEN_CHECK_EQ( InPool, 42ULL );
            ++ReleaseCount;
        }
        UInt32 CreateCount  = 0U;
        UInt32 ReleaseCount = 0U;
    };
} // namespace
LUMEN_TEST( RHIDescriptorAllocatorBase, AllocatesFreesAndReusesContiguousRanges )
{
    FTrackingDescriptorBackend Backend;
    Tests::FNullDescriptorAllocator Allocator( { .InitialCapacity = 4U, .MaximumCapacity = 8U }, Backend, FRHIObjectId{ 1ULL } );
    LUMEN_CHECK_EQ( Backend.CreateCount, 1U );
    auto First  = Allocator.Allocate( 2U );
    auto Second = Allocator.Allocate( 2U );
    LUMEN_CHECK_TRUE( First.HasValue() );
    LUMEN_CHECK_TRUE( Second.HasValue() );
    LUMEN_CHECK_EQ( First->Offset, 0U );
    LUMEN_CHECK_EQ( Second->Offset, 2U );
    auto Third = Allocator.Allocate( 1U );
    LUMEN_CHECK_TRUE( Third.HasValue() );
    LUMEN_CHECK_EQ( Third->Offset, 4U );
    LUMEN_CHECK_EQ( Backend.CreateCount, 2U );
    LUMEN_CHECK_TRUE( Allocator.Free( *First ).HasValue() );
    LUMEN_CHECK_HAS_ERROR( Allocator.Free( *First ) );
    auto Reused = Allocator.Allocate( 2U );
    LUMEN_CHECK_TRUE( Reused.HasValue() );
    LUMEN_CHECK_EQ( Reused->Offset, 0U );
    Allocator.Reset();
    auto AfterReset = Allocator.Allocate( 4U );
    LUMEN_CHECK_TRUE( AfterReset.HasValue() );
    LUMEN_CHECK_EQ( AfterReset->Offset, 0U );
    LUMEN_CHECK_HAS_ERROR( Allocator.Allocate( 0U ) );
    LUMEN_CHECK_HAS_ERROR( Allocator.Free( {} ) );
}
LUMEN_TEST( RHIDescriptorAllocatorBase, ReleasesOwnedBackendPoolsOnDestruction )
{
    FTrackingDescriptorBackend Backend;
    {
        Tests::FNullDescriptorAllocator Allocator( { .InitialCapacity = 4U, .MaximumCapacity = 4U }, Backend,
                                                   FRHIObjectId{ 1ULL } );
        LUMEN_CHECK_EQ( Backend.ReleaseCount, 0U );
    }
    LUMEN_CHECK_EQ( Backend.ReleaseCount, 1U );
}

} // namespace Lumen
