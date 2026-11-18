/**
 * @file RHIResourcesConformanceTests.cpp
 * @brief Runs portable resource creation, mapping, and view scenarios.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/IRHISampler.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHIResourcesConformance, CreatesTextureAndCompatibleView )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            const FRHITextureDesc Desc{ .Extent         = { 16U, 16U, 1U },
                                        .Format         = ERHIFormat::RGBA8Unorm,
                                        .Usage          = ERHIResourceUsage::ShaderResource,
                                        .OptimizedClear = {} };
            const TExpected<TSharedRef<IRHITexture>, FRHIError> Texture = ( *Device )->CreateTexture( Desc );
            LUMEN_CHECK_TRUE( Texture.HasValue() );
            FRHITextureViewDesc ViewDesc;
            ViewDesc.Texture                                             = *Texture;
            const TExpected<TSharedRef<IRHITextureView>, FRHIError> View = ( *Device )->CreateTextureView( ViewDesc );
            LUMEN_CHECK_TRUE( View.HasValue() );
            LUMEN_CHECK_EQ( ( *View )->GetTexture()->GetId().GetValue(), ( *Texture )->GetId().GetValue() );
        }

        LUMEN_TEST( RHIResourcesConformance, MapsCpuBuffersAndCreatesImmutableSamplers )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            FRHIBufferDesc BufferDesc;
            BufferDesc.Size                                           = 64ULL;
            BufferDesc.Usage                                          = ERHIResourceUsage::CopySource;
            BufferDesc.MemoryClass                                    = ERHIMemoryClass::Upload;
            const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer = ( *Device )->CreateBuffer( BufferDesc );
            LUMEN_CHECK_TRUE( Buffer.HasValue() );
            const TExpected<TSpan<Byte>, FRHIError> Mapped = ( *Buffer )->Map( 0ULL, BufferDesc.Size );
            LUMEN_CHECK_TRUE( Mapped.HasValue() );
            LUMEN_CHECK_EQ( Mapped->size(), 64U );
            ( *Mapped )[0] = 0x5AU;
            ( *Buffer )->Unmap();
            const TExpected<TSpan<Byte>, FRHIError> OutOfBounds = ( *Buffer )->Map( 63ULL, 2ULL );
            LUMEN_CHECK_HAS_ERROR( OutOfBounds );
            LUMEN_CHECK_EQ( OutOfBounds.Error().Condition, ERHIErrorCondition::Resource );

            FRHISamplerDesc SamplerDesc;
            const TExpected<TSharedRef<IRHISampler>, FRHIError> Sampler = ( *Device )->CreateSampler( SamplerDesc );
            LUMEN_CHECK_TRUE( Sampler.HasValue() );
            LUMEN_CHECK_TRUE( ( *Sampler )->GetDesc().Validate().HasValue() );
            SamplerDesc.MaxAnisotropy                                          = 0U;
            const TExpected<TSharedRef<IRHISampler>, FRHIError> InvalidSampler = ( *Device )->CreateSampler( SamplerDesc );
            LUMEN_CHECK_HAS_ERROR( InvalidSampler );
            LUMEN_CHECK_EQ( InvalidSampler.Error().Condition, ERHIErrorCondition::Resource );
        }

        LUMEN_TEST( RHIResourcesConformance, RejectsInvalidViewsAndDeviceLocalMappings )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            const TExpected<TSharedRef<IRHITextureView>, FRHIError> EmptyView = ( *Device )->CreateTextureView( {} );
            LUMEN_CHECK_HAS_ERROR( EmptyView );
            LUMEN_CHECK_EQ( EmptyView.Error().Code, ERHIErrorCode::InvalidArgument );
            LUMEN_CHECK_EQ( EmptyView.Error().Condition, ERHIErrorCondition::Resource );

            const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer =
                ( *Device )->CreateBuffer( { .Size = 16ULL, .Usage = ERHIResourceUsage::CopyDestination } );
            LUMEN_CHECK_TRUE( Buffer.HasValue() );
            const TExpected<TSpan<Byte>, FRHIError> Mapping = ( *Buffer )->Map( 0ULL, 1ULL );
            LUMEN_CHECK_HAS_ERROR( Mapping );
            LUMEN_CHECK_EQ( Mapping.Error().Condition, ERHIErrorCondition::Resource );

            const TExpected<TSharedRef<IRHITexture>, FRHIError> Texture =
                ( *Device )->CreateTexture( { .Extent         = { 8U, 8U, 1U },
                                              .Format         = ERHIFormat::RGBA8Unorm,
                                              .Usage          = ERHIResourceUsage::ShaderResource,
                                              .OptimizedClear = {} } );
            LUMEN_CHECK_TRUE( Texture.HasValue() );
            FRHITextureViewDesc ViewDesc;
            ViewDesc.Texture                                                    = *Texture;
            ViewDesc.Range.BaseMip                                              = 1U;
            ViewDesc.Range.MipCount                                             = 1U;
            const TExpected<TSharedRef<IRHITextureView>, FRHIError> InvalidView = ( *Device )->CreateTextureView( ViewDesc );
            LUMEN_CHECK_HAS_ERROR( InvalidView );
            LUMEN_CHECK_EQ( InvalidView.Error().Condition, ERHIErrorCondition::Resource );
        }

        LUMEN_TEST( RHIResourcesConformance, EnforcesPublishedResourceLimits )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            const FRHILimits &Limits = ( *Device )->GetLimits();
            const TExpected<TSharedRef<IRHIBuffer>, FRHIError> OversizedBuffer =
                ( *Device )->CreateBuffer( { .Size = Limits.MaxBufferSize + 1ULL, .Usage = ERHIResourceUsage::CopyDestination } );
            LUMEN_CHECK_HAS_ERROR( OversizedBuffer );
            LUMEN_CHECK_EQ( OversizedBuffer.Error().Code, ERHIErrorCode::Unsupported );
            LUMEN_CHECK_EQ( OversizedBuffer.Error().Condition, ERHIErrorCondition::Limit );

            const TExpected<TSharedRef<IRHITexture>, FRHIError> OversizedTexture =
                ( *Device )->CreateTexture( { .Extent         = { Limits.MaxTextureDimension2D + 1U, 1U, 1U },
                                              .Format         = ERHIFormat::RGBA8Unorm,
                                              .Usage          = ERHIResourceUsage::ShaderResource,
                                              .OptimizedClear = {} } );
            LUMEN_CHECK_HAS_ERROR( OversizedTexture );
            LUMEN_CHECK_EQ( OversizedTexture.Error().Code, ERHIErrorCode::Unsupported );
            LUMEN_CHECK_EQ( OversizedTexture.Error().Condition, ERHIErrorCondition::Limit );
        }

        LUMEN_TEST( RHIResourcesConformance, ViewsRetainTheirParentTexture )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            const TSharedRef<IRHITextureView> View = [&Device]
            {
                const TExpected<TSharedRef<IRHITexture>, FRHIError> Texture =
                    ( *Device )->CreateTexture( { .Extent         = { 8U, 8U, 1U },
                                                  .Format         = ERHIFormat::RGBA8Unorm,
                                                  .Usage          = ERHIResourceUsage::ShaderResource,
                                                  .OptimizedClear = {} } );
                LUMEN_CHECK_TRUE( Texture.HasValue() );
                FRHITextureViewDesc ViewDesc;
                ViewDesc.Texture                                                   = *Texture;
                const TExpected<TSharedRef<IRHITextureView>, FRHIError> ViewResult = ( *Device )->CreateTextureView( ViewDesc );
                LUMEN_CHECK_TRUE( ViewResult.HasValue() );
                return *ViewResult;
            }();
            LUMEN_CHECK_TRUE( View->GetTexture().Get() != nullptr );
            LUMEN_CHECK_EQ( View->GetTexture()->GetDesc().Extent.Width, 8U );
            LUMEN_CHECK_EQ( View->GetDesc().Texture.Get(), View->GetTexture().Get() );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
