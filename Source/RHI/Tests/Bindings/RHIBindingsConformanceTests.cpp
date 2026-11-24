/**
 * @file RHIBindingsConformanceTests.cpp
 * @brief Runs portable binding and descriptor creation scenarios.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"
#include <RHI/Bindings/IRHIBindingSet.hpp>
#include <RHI/Bindings/IRHIDescriptorAllocator.hpp>
#include <RHI/Bindings/RHIDescriptorAllocatorDesc.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Resources/RHISamplerDesc.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Resources/RHITextureViewDesc.hpp>
#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    namespace
    {
        LUMEN_TEST( RHIBindingsConformance, CreatesLayoutSetAndDescriptorAllocator )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            FRHIBindingLayoutDesc LayoutDesc;
            LayoutDesc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer } );
            TExpected<TSharedRef<IRHIBindingLayout>, FRHIError> Layout = ( *Device )->CreateBindingLayout( LayoutDesc );
            LUMEN_CHECK_TRUE( Layout.HasValue() );
            TExpected<TSharedRef<IRHIBindingSet>, FRHIError> Set = ( *Device )->CreateBindingSet( *Layout );
            LUMEN_CHECK_TRUE( Set.HasValue() );
            LUMEN_CHECK_EQ( ( *Set )->GetLayout()->GetId().GetValue(), ( *Layout )->GetId().GetValue() );
            TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer =
                ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::ShaderResource } );
            LUMEN_CHECK_TRUE( Buffer.HasValue() );
            const FRHIBindingWrite Write{ .Slot = 0U, .Resource = FRHIBufferBinding{ .Buffer = *Buffer } };
            LUMEN_CHECK_TRUE( ( *Set )->Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) ).HasValue() );
            LUMEN_CHECK_TRUE( ( *Set )->IsDirty() );
            ( *Set )->ClearDirty();
            LUMEN_CHECK_FALSE( ( *Set )->IsDirty() );
            TExpected<TSharedRef<IRHIDescriptorAllocator>, FRHIError> Allocator =
                ( *Device )->CreateDescriptorAllocator( { .InitialCapacity = 4U, .MaximumCapacity = 8U } );
            LUMEN_CHECK_TRUE( Allocator.HasValue() );
            TExpected<FRHIDescriptorAllocation, FRHIError> Allocation = ( *Allocator )->Allocate( 2U );
            LUMEN_CHECK_TRUE( Allocation.HasValue() );
            LUMEN_CHECK_EQ( Allocation->Count, 2U );
            LUMEN_CHECK_TRUE( ( *Allocator )->Free( *Allocation ).HasValue() );
        }
        LUMEN_TEST( RHIBindingsConformance, RejectsInvalidLayoutsAndAllocatorPolicies )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            FRHIBindingLayoutDesc LayoutDesc;
            LayoutDesc.Entries.push_back( { .Count = 0U } );
            LUMEN_CHECK_HAS_ERROR( ( *Device )->CreateBindingLayout( LayoutDesc ) );
            LUMEN_CHECK_HAS_ERROR( ( *Device )->CreateDescriptorAllocator( { .InitialCapacity = 8U, .MaximumCapacity = 4U } ) );
        }

        LUMEN_TEST( RHIBindingsConformance, RejectsInvalidWritesAtomically )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            FRHIBindingLayoutDesc LayoutDesc;
            LayoutDesc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer } );
            TExpected<TSharedRef<IRHIBindingLayout>, FRHIError> Layout = ( *Device )->CreateBindingLayout( LayoutDesc );
            LUMEN_CHECK_TRUE( Layout.HasValue() );
            TExpected<TSharedRef<IRHIBindingSet>, FRHIError> Set = ( *Device )->CreateBindingSet( *Layout );
            LUMEN_CHECK_TRUE( Set.HasValue() );
            ( *Set )->ClearDirty();

            TExpected<TSharedRef<IRHISampler>, FRHIError> Sampler = ( *Device )->CreateSampler( FRHISamplerDesc{} );
            LUMEN_CHECK_TRUE( Sampler.HasValue() );
            const FRHIBindingWrite WrongType{ .Slot = 0U, .Resource = FRHISamplerBinding{ .Sampler = *Sampler } };
            const FRHIExpected WrongTypeResult = ( *Set )->Write( TSpan<const FRHIBindingWrite>( &WrongType, 1U ) );
            LUMEN_CHECK_HAS_ERROR( WrongTypeResult );
            LUMEN_CHECK_EQ( WrongTypeResult.Error().Condition, ERHIErrorCondition::Binding );
            LUMEN_CHECK_FALSE( ( *Set )->IsDirty() );

            TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer =
                ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::ShaderResource } );
            LUMEN_CHECK_TRUE( Buffer.HasValue() );
            const FRHIBindingWrite UnknownSlot{ .Slot = 99U, .Resource = FRHIBufferBinding{ .Buffer = *Buffer } };
            const FRHIExpected UnknownSlotResult = ( *Set )->Write( TSpan<const FRHIBindingWrite>( &UnknownSlot, 1U ) );
            LUMEN_CHECK_HAS_ERROR( UnknownSlotResult );
            LUMEN_CHECK_FALSE( ( *Set )->IsDirty() );
        }

        LUMEN_TEST( RHIBindingsConformance, DescriptorRangesExhaustRecycleAndReset )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            TExpected<TSharedRef<IRHIDescriptorAllocator>, FRHIError> Allocator =
                ( *Device )->CreateDescriptorAllocator( { .InitialCapacity = 4U, .MaximumCapacity = 8U } );
            LUMEN_CHECK_TRUE( Allocator.HasValue() );
            TExpected<FRHIDescriptorAllocation, FRHIError> First = ( *Allocator )->Allocate( 4U );
            LUMEN_CHECK_TRUE( First.HasValue() );
            TExpected<FRHIDescriptorAllocation, FRHIError> Growth = ( *Allocator )->Allocate( 4U );
            LUMEN_CHECK_TRUE( Growth.HasValue() );
            LUMEN_CHECK_EQ( Growth->Offset, 4U );
            const TExpected<FRHIDescriptorAllocation, FRHIError> Exhausted = ( *Allocator )->Allocate( 1U );
            LUMEN_CHECK_HAS_ERROR( Exhausted );
            LUMEN_CHECK_EQ( Exhausted.Error().Code, ERHIErrorCode::OutOfMemory );
            LUMEN_CHECK_EQ( Exhausted.Error().Condition, ERHIErrorCondition::Binding );
            LUMEN_CHECK_TRUE( ( *Allocator )->Free( *First ).HasValue() );
            TExpected<FRHIDescriptorAllocation, FRHIError> Reused = ( *Allocator )->Allocate( 4U );
            LUMEN_CHECK_TRUE( Reused.HasValue() );
            LUMEN_CHECK_EQ( Reused->Offset, First->Offset );
            LUMEN_CHECK_TRUE( ( *Allocator )->Free( *Reused ).HasValue() );
            LUMEN_CHECK_TRUE( ( *Allocator )->Free( *Growth ).HasValue() );
            ( *Allocator )->Reset();
            TExpected<FRHIDescriptorAllocation, FRHIError> AfterReset = ( *Allocator )->Allocate( 4U );
            LUMEN_CHECK_TRUE( AfterReset.HasValue() );
            LUMEN_CHECK_EQ( AfterReset->Offset, 0U );
            LUMEN_CHECK_TRUE( ( *Allocator )->Free( *AfterReset ).HasValue() );
        }

        LUMEN_TEST( RHIBindingsConformance, BindlessLayoutsFollowCapabilityNegotiation )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            FRHIBindingLayoutDesc LayoutDesc;
            LayoutDesc.Entries.push_back( { .Slot = 0U, .bBindless = true } );
            TExpected<TSharedRef<IRHIBindingLayout>, FRHIError> Layout = ( *Device )->CreateBindingLayout( LayoutDesc );
            if ( ( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::BindlessResources ) )
            {
                LUMEN_CHECK_TRUE( Layout.HasValue() );
                return;
            }
            LUMEN_CHECK_HAS_ERROR( Layout );
            LUMEN_CHECK_EQ( Layout.Error().Code, ERHIErrorCode::Unsupported );
            LUMEN_CHECK_EQ( Layout.Error().Condition, ERHIErrorCondition::Capability );
        }

        LUMEN_TEST( RHIBindingsConformance, WritesArrayElementsAndCombinedResourcesAtomically )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice( true );
            LUMEN_CHECK_TRUE( Device.HasValue() );
            FRHIBindingLayoutDesc LayoutDesc;
            LayoutDesc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer, .Count = 2U } );
            LayoutDesc.Entries.push_back( { .Slot = 1U, .Type = ERHIBindingType::CombinedTextureSampler } );
            TExpected<TSharedRef<IRHIBindingLayout>, FRHIError> Layout = ( *Device )->CreateBindingLayout( LayoutDesc );
            LUMEN_CHECK_TRUE( Layout.HasValue() );
            TExpected<TSharedRef<IRHIBindingSet>, FRHIError> Set = ( *Device )->CreateBindingSet( *Layout );
            LUMEN_CHECK_TRUE( Set.HasValue() );

            TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer =
                ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::ShaderResource } );
            TExpected<TSharedRef<IRHITexture>, FRHIError> Texture =
                ( *Device )->CreateTexture( { .Extent         = { 4U, 4U, 1U },
                                              .Format         = ERHIFormat::RGBA8Unorm,
                                              .Usage          = ERHIResourceUsage::ShaderResource,
                                              .OptimizedClear = {} } );
            LUMEN_CHECK_TRUE( Buffer.HasValue() );
            LUMEN_CHECK_TRUE( Texture.HasValue() );
            FRHITextureViewDesc ViewDesc;
            ViewDesc.Texture                                       = *Texture;
            TExpected<TSharedRef<IRHITextureView>, FRHIError> View = ( *Device )->CreateTextureView( ViewDesc );
            TExpected<TSharedRef<IRHISampler>, FRHIError> Sampler  = ( *Device )->CreateSampler( {} );
            LUMEN_CHECK_TRUE( View.HasValue() );
            LUMEN_CHECK_TRUE( Sampler.HasValue() );

            const FRHIBindingWrite ArrayWrite{
                .Slot = 0U, .ArrayElement = 1U, .Resource = FRHIBufferBinding{ .Buffer = *Buffer } };
            const FRHIBindingWrite CombinedWrite{
                .Slot = 1U, .Resource = FRHICombinedTextureSamplerBinding{ .View = *View, .Sampler = *Sampler } };
            const TArray<FRHIBindingWrite, 2U> Writes = { ArrayWrite, CombinedWrite };
            LUMEN_CHECK_TRUE( ( *Set )->Write( Writes ).HasValue() );
            ( *Set )->ClearDirty();

            FRHIBindingWrite InvalidArray = ArrayWrite;
            InvalidArray.ArrayElement     = 2U;
            LUMEN_CHECK_HAS_ERROR( ( *Set )->Write( TSpan<const FRHIBindingWrite>( &InvalidArray, 1U ) ) );
            LUMEN_CHECK_FALSE( ( *Set )->IsDirty() );
            const FRHIBindingWrite MissingSampler{ .Slot = 1U, .Resource = FRHITextureBinding{ .View = *View } };
            LUMEN_CHECK_HAS_ERROR( ( *Set )->Write( TSpan<const FRHIBindingWrite>( &MissingSampler, 1U ) ) );
            LUMEN_CHECK_FALSE( ( *Set )->IsDirty() );
        }

    } // namespace

} // namespace RHI

} // namespace Lumen
