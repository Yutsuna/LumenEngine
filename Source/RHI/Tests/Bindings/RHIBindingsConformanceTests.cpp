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
#include <Resources/RHISamplerDesc.hpp>
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

    LUMEN_TEST( RHIBindingsConformance, RejectsInvalidWritesAtomically )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        FRHIBindingLayoutDesc LayoutDesc;
        LayoutDesc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer } );
        auto Layout = ( *Device )->CreateBindingLayout( LayoutDesc );
        LUMEN_CHECK_TRUE( Layout.HasValue() );
        auto Set = ( *Device )->CreateBindingSet( *Layout );
        LUMEN_CHECK_TRUE( Set.HasValue() );
        ( *Set )->ClearDirty();

        auto Sampler = ( *Device )->CreateSampler( FRHISamplerDesc{} );
        LUMEN_CHECK_TRUE( Sampler.HasValue() );
        const FRHIBindingWrite WrongType{ .Slot = 0U, .Resource = FRHISamplerBinding{ .Sampler = *Sampler } };
        const auto WrongTypeResult = ( *Set )->Write( TSpan<const FRHIBindingWrite>( &WrongType, 1U ) );
        LUMEN_CHECK_HAS_ERROR( WrongTypeResult );
        LUMEN_CHECK_EQ( WrongTypeResult.Error().Condition, ERHIErrorCondition::Binding );
        LUMEN_CHECK_FALSE( ( *Set )->IsDirty() );

        auto Buffer = ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::ShaderResource } );
        LUMEN_CHECK_TRUE( Buffer.HasValue() );
        const FRHIBindingWrite UnknownSlot{ .Slot = 99U, .Resource = FRHIBufferBinding{ .Buffer = *Buffer } };
        const auto UnknownSlotResult = ( *Set )->Write( TSpan<const FRHIBindingWrite>( &UnknownSlot, 1U ) );
        LUMEN_CHECK_HAS_ERROR( UnknownSlotResult );
        LUMEN_CHECK_FALSE( ( *Set )->IsDirty() );
    }

    LUMEN_TEST( RHIBindingsConformance, DescriptorRangesExhaustRecycleAndReset )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        auto Allocator = ( *Device )->CreateDescriptorAllocator( { .InitialCapacity = 4U, .MaximumCapacity = 8U } );
        LUMEN_CHECK_TRUE( Allocator.HasValue() );
        auto First = ( *Allocator )->Allocate( 4U );
        LUMEN_CHECK_TRUE( First.HasValue() );
        auto Growth = ( *Allocator )->Allocate( 4U );
        LUMEN_CHECK_TRUE( Growth.HasValue() );
        LUMEN_CHECK_EQ( Growth->Offset, 4U );
        const auto Exhausted = ( *Allocator )->Allocate( 1U );
        LUMEN_CHECK_HAS_ERROR( Exhausted );
        LUMEN_CHECK_EQ( Exhausted.Error().Code, ERHIErrorCode::OutOfMemory );
        LUMEN_CHECK_EQ( Exhausted.Error().Condition, ERHIErrorCondition::Binding );
        LUMEN_CHECK_TRUE( ( *Allocator )->Free( *First ).HasValue() );
        auto Reused = ( *Allocator )->Allocate( 4U );
        LUMEN_CHECK_TRUE( Reused.HasValue() );
        LUMEN_CHECK_EQ( Reused->Offset, First->Offset );
        LUMEN_CHECK_TRUE( ( *Allocator )->Free( *Reused ).HasValue() );
        LUMEN_CHECK_TRUE( ( *Allocator )->Free( *Growth ).HasValue() );
        ( *Allocator )->Reset();
        auto AfterReset = ( *Allocator )->Allocate( 4U );
        LUMEN_CHECK_TRUE( AfterReset.HasValue() );
        LUMEN_CHECK_EQ( AfterReset->Offset, 0U );
        LUMEN_CHECK_TRUE( ( *Allocator )->Free( *AfterReset ).HasValue() );
    }

    LUMEN_TEST( RHIBindingsConformance, BindlessLayoutsFollowCapabilityNegotiation )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        FRHIBindingLayoutDesc LayoutDesc;
        LayoutDesc.Entries.push_back( { .Slot = 0U, .bBindless = true } );
        auto Layout = ( *Device )->CreateBindingLayout( LayoutDesc );
        if ( ( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::BindlessResources ) )
        {
            LUMEN_CHECK_TRUE( Layout.HasValue() );
            return;
        }
        LUMEN_CHECK_HAS_ERROR( Layout );
        LUMEN_CHECK_EQ( Layout.Error().Code, ERHIErrorCode::Unsupported );
        LUMEN_CHECK_EQ( Layout.Error().Condition, ERHIErrorCondition::Capability );
    }

} // namespace

} // namespace Lumen
