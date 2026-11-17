/**
 * @file RHIResourcesConformanceTests.cpp
 * @brief Runs portable resource creation, mapping, and view scenarios.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"
#include <Device/IRHIDevice.hpp>
#include <Resources/IRHIBuffer.hpp>
#include <Resources/IRHISampler.hpp>
#include <Resources/IRHITexture.hpp>
#include <Resources/IRHITextureView.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIResourcesConformance, CreatesTextureAndCompatibleView )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const FRHITextureDesc Desc{ .Extent         = { 16U, 16U, 1U },
                                    .Format         = ERHIFormat::RGBA8Unorm,
                                    .Usage          = ERHIResourceUsage::ShaderResource,
                                    .OptimizedClear = {} };
        auto Texture = ( *Device )->CreateTexture( Desc );
        LUMEN_CHECK_TRUE( Texture.HasValue() );
        FRHITextureViewDesc ViewDesc;
        ViewDesc.Texture = *Texture;
        auto View        = ( *Device )->CreateTextureView( ViewDesc );
        LUMEN_CHECK_TRUE( View.HasValue() );
        LUMEN_CHECK_EQ( ( *View )->GetTexture()->GetId().GetValue(), ( *Texture )->GetId().GetValue() );
    }

    LUMEN_TEST( RHIResourcesConformance, MapsCpuBuffersAndCreatesImmutableSamplers )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        FRHIBufferDesc BufferDesc;
        BufferDesc.Size        = 64ULL;
        BufferDesc.Usage       = ERHIResourceUsage::CopySource;
        BufferDesc.MemoryClass = ERHIMemoryClass::Upload;
        auto Buffer            = ( *Device )->CreateBuffer( BufferDesc );
        LUMEN_CHECK_TRUE( Buffer.HasValue() );
        auto Mapped = ( *Buffer )->Map( 0ULL, BufferDesc.Size );
        LUMEN_CHECK_TRUE( Mapped.HasValue() );
        LUMEN_CHECK_EQ( Mapped->size(), 64U );
        ( *Mapped )[0] = 0x5AU;
        ( *Buffer )->Unmap();
        const auto OutOfBounds = ( *Buffer )->Map( 63ULL, 2ULL );
        LUMEN_CHECK_HAS_ERROR( OutOfBounds );
        LUMEN_CHECK_EQ( OutOfBounds.Error().Condition, ERHIErrorCondition::Resource );

        FRHISamplerDesc SamplerDesc;
        auto Sampler = ( *Device )->CreateSampler( SamplerDesc );
        LUMEN_CHECK_TRUE( Sampler.HasValue() );
        LUMEN_CHECK_TRUE( ( *Sampler )->GetDesc().Validate().HasValue() );
        SamplerDesc.MaxAnisotropy = 0U;
        const auto InvalidSampler = ( *Device )->CreateSampler( SamplerDesc );
        LUMEN_CHECK_HAS_ERROR( InvalidSampler );
        LUMEN_CHECK_EQ( InvalidSampler.Error().Condition, ERHIErrorCondition::Resource );
    }

    LUMEN_TEST( RHIResourcesConformance, RejectsInvalidViewsAndDeviceLocalMappings )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const auto EmptyView = ( *Device )->CreateTextureView( {} );
        LUMEN_CHECK_HAS_ERROR( EmptyView );
        LUMEN_CHECK_EQ( EmptyView.Error().Code, ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( EmptyView.Error().Condition, ERHIErrorCondition::Resource );

        auto Buffer = ( *Device )->CreateBuffer( { .Size = 16ULL, .Usage = ERHIResourceUsage::CopyDestination } );
        LUMEN_CHECK_TRUE( Buffer.HasValue() );
        const auto Mapping = ( *Buffer )->Map( 0ULL, 1ULL );
        LUMEN_CHECK_HAS_ERROR( Mapping );
        LUMEN_CHECK_EQ( Mapping.Error().Condition, ERHIErrorCondition::Resource );

        auto Texture = ( *Device )->CreateTexture( { .Extent         = { 8U, 8U, 1U },
                                                     .Format         = ERHIFormat::RGBA8Unorm,
                                                     .Usage          = ERHIResourceUsage::ShaderResource,
                                                     .OptimizedClear = {} } );
        LUMEN_CHECK_TRUE( Texture.HasValue() );
        FRHITextureViewDesc ViewDesc;
        ViewDesc.Texture        = *Texture;
        ViewDesc.Range.BaseMip  = 1U;
        ViewDesc.Range.MipCount = 1U;
        const auto InvalidView  = ( *Device )->CreateTextureView( ViewDesc );
        LUMEN_CHECK_HAS_ERROR( InvalidView );
        LUMEN_CHECK_EQ( InvalidView.Error().Condition, ERHIErrorCondition::Resource );
    }

    LUMEN_TEST( RHIResourcesConformance, EnforcesPublishedResourceLimits )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const FRHILimits &Limits = ( *Device )->GetLimits();
        const auto OversizedBuffer =
            ( *Device )->CreateBuffer( { .Size = Limits.MaxBufferSize + 1ULL, .Usage = ERHIResourceUsage::CopyDestination } );
        LUMEN_CHECK_HAS_ERROR( OversizedBuffer );
        LUMEN_CHECK_EQ( OversizedBuffer.Error().Code, ERHIErrorCode::Unsupported );
        LUMEN_CHECK_EQ( OversizedBuffer.Error().Condition, ERHIErrorCondition::Limit );

        const auto OversizedTexture = ( *Device )->CreateTexture( { .Extent = { Limits.MaxTextureDimension2D + 1U, 1U, 1U },
                                                                    .Format = ERHIFormat::RGBA8Unorm,
                                                                    .Usage  = ERHIResourceUsage::ShaderResource,
                                                                    .OptimizedClear = {} } );
        LUMEN_CHECK_HAS_ERROR( OversizedTexture );
        LUMEN_CHECK_EQ( OversizedTexture.Error().Code, ERHIErrorCode::Unsupported );
        LUMEN_CHECK_EQ( OversizedTexture.Error().Condition, ERHIErrorCondition::Limit );
    }

    LUMEN_TEST( RHIResourcesConformance, ViewsRetainTheirParentTexture )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const auto View = [&Device]
        {
            auto Texture = ( *Device )->CreateTexture( { .Extent         = { 8U, 8U, 1U },
                                                         .Format         = ERHIFormat::RGBA8Unorm,
                                                         .Usage          = ERHIResourceUsage::ShaderResource,
                                                         .OptimizedClear = {} } );
            LUMEN_CHECK_TRUE( Texture.HasValue() );
            FRHITextureViewDesc ViewDesc;
            ViewDesc.Texture = *Texture;
            auto ViewResult  = ( *Device )->CreateTextureView( ViewDesc );
            LUMEN_CHECK_TRUE( ViewResult.HasValue() );
            return *ViewResult;
        }();
        LUMEN_CHECK_TRUE( View->GetTexture().Get() != nullptr );
        LUMEN_CHECK_EQ( View->GetTexture()->GetDesc().Extent.Width, 8U );
        LUMEN_CHECK_EQ( View->GetDesc().Texture.Get(), View->GetTexture().Get() );
    }

} // namespace
} // namespace Lumen
