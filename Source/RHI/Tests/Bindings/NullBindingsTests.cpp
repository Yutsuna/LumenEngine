/**
 * @file NullBindingsTests.cpp
 * @brief Tests Null binding layouts, writes, and dirty tracking.
 */

#include "Bindings/NullBindings.hpp"
#include "Device/NullDevice.hpp"
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    namespace
    {
        LUMEN_TEST( NullBindings, AppliesKnownWritesAndTracksDirtyState )
        {
            FRHIBindingLayoutDesc Desc;
            Desc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer } );
            TSharedRef<Tests::FNullBindingLayout> Layout = MakeShared<Tests::FNullBindingLayout>( Desc, FRHIObjectId{ 1ULL } );
            Tests::FNullBindingSet Set( Layout, FRHIObjectId{ 2ULL } );
            LUMEN_CHECK_EQ( Set.GetId().GetValue(), 2ULL );
            LUMEN_CHECK_EQ( Set.GetLayout()->GetId().GetValue(), 1ULL );
            LUMEN_CHECK_FALSE( Set.IsDirty() );
            Tests::FNullDevice Device;
            TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer =
                Device.CreateBuffer( { .Size = 32ULL, .Usage = ERHIResourceUsage::ShaderResource } );
            LUMEN_CHECK_TRUE( Buffer.HasValue() );
            const FRHIBindingWrite Write{ .Slot = 0U, .Resource = FRHIBufferBinding{ .Buffer = *Buffer } };
            LUMEN_CHECK_TRUE( Set.Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) ).HasValue() );
            LUMEN_CHECK_TRUE( Set.IsDirty() );
            Set.ClearDirty();
            LUMEN_CHECK_FALSE( Set.IsDirty() );
        }
        LUMEN_TEST( NullBindings, RejectsUnknownSlotsAndNullResourcesWithoutDirtying )
        {
            FRHIBindingLayoutDesc Desc;
            Desc.Entries.push_back( { .Slot = 3U } );
            TSharedRef<Tests::FNullBindingLayout> Layout = MakeShared<Tests::FNullBindingLayout>( Desc, FRHIObjectId{ 1ULL } );
            Tests::FNullBindingSet Set( Layout, FRHIObjectId{ 2ULL } );
            const FRHIBindingWrite UnknownSlot{ .Slot = 4U, .Resource = {} };
            LUMEN_CHECK_HAS_ERROR( Set.Write( TSpan<const FRHIBindingWrite>( &UnknownSlot, 1U ) ) );
            const FRHIBindingWrite NullResource{ .Slot = 3U, .Resource = {} };
            LUMEN_CHECK_HAS_ERROR( Set.Write( TSpan<const FRHIBindingWrite>( &NullResource, 1U ) ) );
            LUMEN_CHECK_FALSE( Set.IsDirty() );
        }

    } // namespace

} // namespace RHI

} // namespace Lumen
