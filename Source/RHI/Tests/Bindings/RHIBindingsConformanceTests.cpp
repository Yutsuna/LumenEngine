/**
 * @file RHIBindingsConformanceTests.cpp
 * @brief Runs portable binding and descriptor creation scenarios.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"
#include <Bindings/IRHIBindingSet.hpp>
#include <Bindings/IRHIDescriptorAllocator.hpp>
#include <Bindings/RHIDescriptorAllocatorDesc.hpp>
#include <Device/IRHIDevice.hpp>
#include <Resources/RHIBufferDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{
    LUMEN_TEST( RHIBindingsConformance, CreatesLayoutSetAndDescriptorAllocator )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        FRHIBindingLayoutDesc LayoutDesc;
        LayoutDesc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer } );
        auto Layout = ( *Device )->CreateBindingLayout( LayoutDesc );
        LUMEN_CHECK_TRUE( Layout.HasValue() );
        auto Set = ( *Device )->CreateBindingSet( *Layout );
        LUMEN_CHECK_TRUE( Set.HasValue() );
        LUMEN_CHECK_EQ( ( *Set )->GetLayout()->GetId().GetValue(), ( *Layout )->GetId().GetValue() );
        auto Buffer = ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::ShaderResource } );
        LUMEN_CHECK_TRUE( Buffer.HasValue() );
        const FRHIBindingWrite Write{ .Slot = 0U, .Resource = FRHIBufferBinding{ .Buffer = *Buffer } };
        LUMEN_CHECK_TRUE( ( *Set )->Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) ).HasValue() );
        LUMEN_CHECK_TRUE( ( *Set )->IsDirty() );
        ( *Set )->ClearDirty();
        LUMEN_CHECK_FALSE( ( *Set )->IsDirty() );
        auto Allocator = ( *Device )->CreateDescriptorAllocator( { .InitialCapacity = 4U, .MaximumCapacity = 8U } );
        LUMEN_CHECK_TRUE( Allocator.HasValue() );
        auto Allocation = ( *Allocator )->Allocate( 2U );
        LUMEN_CHECK_TRUE( Allocation.HasValue() );
        LUMEN_CHECK_EQ( Allocation->Count, 2U );
        LUMEN_CHECK_TRUE( ( *Allocator )->Free( *Allocation ).HasValue() );
    }
    LUMEN_TEST( RHIBindingsConformance, RejectsInvalidLayoutsAndAllocatorPolicies )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        FRHIBindingLayoutDesc LayoutDesc;
        LayoutDesc.Entries.push_back( { .Count = 0U } );
        LUMEN_CHECK_HAS_ERROR( ( *Device )->CreateBindingLayout( LayoutDesc ) );
        LUMEN_CHECK_HAS_ERROR( ( *Device )->CreateDescriptorAllocator( { .InitialCapacity = 8U, .MaximumCapacity = 4U } ) );
    }

} // namespace

} // namespace Lumen
