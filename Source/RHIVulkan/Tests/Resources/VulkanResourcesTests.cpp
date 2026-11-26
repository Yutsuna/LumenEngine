/**
 * @file VulkanResourcesTests.cpp
 * @brief Integration tests for Vulkan buffers, textures, views and samplers.
 */

#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Memory/IRHIHeap.hpp>
#include <RHI/Memory/RHIAllocation.hpp>
#include <RHI/Memory/RHIHeapDesc.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/IRHISampler.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHIVulkan/RHIVulkanModule.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    namespace
    {

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> CreateTestDevice () noexcept
        {
            const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
            if ( not Module )
            {
                return TUnexpected( Module.Error() );
            }
            const TExpected<TVector<RHI::FRHIAdapterInfo>, RHI::FRHIError> Adapters = ( *Module )->EnumerateAdapters();
            if ( not Adapters )
            {
                return TUnexpected( Adapters.Error() );
            }
            if ( Adapters->empty() )
            {
                return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Device, "No Vulkan adapter available." ) );
            }
            RHI::FRHIDeviceDesc Desc;
            Desc.AdapterId.Emplace( ( *Adapters )[0].AdapterId );
            return ( *Module )->CreateDevice( Desc );
        }

    } // namespace

    LUMEN_TEST( VulkanResources, CreatesBuffersTexturesViewsAndSamplers )
    {
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = CreateTestDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );

        const RHI::FRHIBufferDesc BufferDesc{
            .Size = 64ULL, .Usage = RHI::ERHIResourceUsage::CopySource, .MemoryClass = RHI::ERHIMemoryClass::Upload };
        const TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError> Buffer = ( *Device )->CreateBuffer( BufferDesc );
        LUMEN_CHECK_TRUE( Buffer.HasValue() );
        LUMEN_CHECK_TRUE( ( *Buffer )->GetId().IsValid() );
        const TExpected<TSpan<Byte>, RHI::FRHIError> Mapped = ( *Buffer )->Map( 0ULL, 64ULL );
        LUMEN_CHECK_TRUE( Mapped.HasValue() );
        ( *Mapped )[0] = Byte{ 0x5AU };
        ( *Buffer )->Unmap();
        LUMEN_CHECK_HAS_ERROR( ( *Buffer )->Map( 63ULL, 2ULL ) );

        const RHI::FRHITextureDesc TextureDesc{ .Extent         = { 16U, 16U, 1U },
                                                .Format         = RHI::ERHIFormat::RGBA8Unorm,
                                                .Usage          = RHI::ERHIResourceUsage::ShaderResource,
                                                .OptimizedClear = {} };
        const TExpected<TSharedRef<RHI::IRHITexture>, RHI::FRHIError> Texture = ( *Device )->CreateTexture( TextureDesc );
        LUMEN_CHECK_TRUE( Texture.HasValue() );
        RHI::FRHITextureViewDesc ViewDesc;
        ViewDesc.Texture                                                       = *Texture;
        const TExpected<TSharedRef<RHI::IRHITextureView>, RHI::FRHIError> View = ( *Device )->CreateTextureView( ViewDesc );
        LUMEN_CHECK_TRUE( View.HasValue() );
        LUMEN_CHECK_EQ( ( *View )->GetTexture()->GetId().GetValue(), ( *Texture )->GetId().GetValue() );

        RHI::FRHITextureViewDesc InvalidView = ViewDesc;
        InvalidView.Range.BaseMip            = 1U;
        LUMEN_CHECK_HAS_ERROR( ( *Device )->CreateTextureView( InvalidView ) );

        RHI::FRHISamplerDesc SamplerDesc;
        const TExpected<TSharedRef<RHI::IRHISampler>, RHI::FRHIError> Sampler = ( *Device )->CreateSampler( SamplerDesc );
        LUMEN_CHECK_TRUE( Sampler.HasValue() );
        SamplerDesc.MaxAnisotropy = 0U;
        LUMEN_CHECK_HAS_ERROR( ( *Device )->CreateSampler( SamplerDesc ) );
    }

    LUMEN_TEST( VulkanResources, PlacesResourcesAndRetainsTheirHeap )
    {
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = CreateTestDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const TExpected<TSharedRef<RHI::IRHIHeap>, RHI::FRHIError> Heap =
            ( *Device )->CreateHeap( { .Size = 8192ULL, .MemoryClass = RHI::ERHIMemoryClass::Upload } );
        LUMEN_CHECK_TRUE( Heap.HasValue() );

        const RHI::FRHIAllocation Allocation{ .Id          = RHI::FRHIAllocationId{ 1ULL },
                                              .Offset      = 256ULL,
                                              .Size        = 512ULL,
                                              .Alignment   = 256ULL,
                                              .MemoryClass = RHI::ERHIMemoryClass::Upload,
                                              .Heap        = ( *Heap ).Get() };
        const RHI::FRHIBufferDesc BufferDesc{
            .Size = 64ULL, .Usage = RHI::ERHIResourceUsage::CopySource, .MemoryClass = RHI::ERHIMemoryClass::Upload };
        const TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError> Buffer =
            ( *Device )->CreatePlacedBuffer( BufferDesc, *Heap, Allocation );
        LUMEN_CHECK_TRUE( Buffer.HasValue() );
        const TExpected<TSpan<Byte>, RHI::FRHIError> Mapped = ( *Buffer )->Map( 0ULL, 64ULL );
        LUMEN_CHECK_TRUE( Mapped.HasValue() );
        ( *Mapped )[0] = Byte{ 0x5AU };
        ( *Buffer )->Unmap();
        const TExpected<TSpan<Byte>, RHI::FRHIError> HeapBytes = ( *Heap )->Map( 256ULL, 1ULL );
        LUMEN_CHECK_TRUE( HeapBytes.HasValue() );
        LUMEN_CHECK_EQ( static_cast<UInt32>( ( *HeapBytes )[0] ), 0x5AU );
        ( *Heap )->Unmap();

        const RHI::FRHITextureDesc TextureDesc{ .Extent         = { 4U, 4U, 1U },
                                                .Format         = RHI::ERHIFormat::RGBA8Unorm,
                                                .Usage          = RHI::ERHIResourceUsage::CopySource,
                                                .MemoryClass    = RHI::ERHIMemoryClass::Upload,
                                                .OptimizedClear = {} };
        const RHI::FRHIAllocation TextureAllocation{ .Id          = RHI::FRHIAllocationId{ 2ULL },
                                                     .Offset      = 2048ULL,
                                                     .Size        = 2048ULL,
                                                     .Alignment   = 256ULL,
                                                     .MemoryClass = RHI::ERHIMemoryClass::Upload,
                                                     .Heap        = ( *Heap ).Get() };
        LUMEN_CHECK_TRUE( ( *Device )->CreatePlacedTexture( TextureDesc, *Heap, TextureAllocation ).HasValue() );

        RHI::FRHIAllocation Misaligned = Allocation;
        Misaligned.Offset              = 2ULL;
        LUMEN_CHECK_HAS_ERROR( ( *Device )->CreatePlacedBuffer( BufferDesc, *Heap, Misaligned ) );
        RHI::FRHIAllocation WrongHeap = Allocation;
        WrongHeap.Heap                = nullptr;
        LUMEN_CHECK_HAS_ERROR( ( *Device )->CreatePlacedBuffer( BufferDesc, *Heap, WrongHeap ) );
    }

} // namespace RHIVulkan

} // namespace Lumen
