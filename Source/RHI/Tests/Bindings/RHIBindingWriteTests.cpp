/**
 * @file RHIBindingWriteTests.cpp
 * @brief Tests typed binding resource validation.
 */

#include "Device/NullDevice.hpp"
#include <RHI/Bindings/RHIBindingWrite.hpp>
#include <RHI/Resources/IRHISampler.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    namespace
    {
        LUMEN_TEST( RHIBindingWrite, RequiresNonNullResourceForEveryPayloadKind )
        {
            FRHIBindingWrite Write;
            LUMEN_CHECK_HAS_ERROR( Write.Validate() );
            Tests::FNullDevice Device;
            TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer =
                Device.CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::ShaderResource } );
            LUMEN_CHECK_TRUE( Buffer.HasValue() );
            Write.Resource = FRHIBufferBinding{ .Buffer = *Buffer, .Offset = 16ULL, .Size = 32ULL };
            LUMEN_CHECK_TRUE( Write.Validate().HasValue() );
            TExpected<TSharedRef<IRHITexture>, FRHIError> Texture =
                Device.CreateTexture( { .Extent         = { 4U, 4U, 1U },
                                        .Format         = ERHIFormat::RGBA8Unorm,
                                        .Usage          = ERHIResourceUsage::ShaderResource,
                                        .OptimizedClear = {} } );
            LUMEN_CHECK_TRUE( Texture.HasValue() );
            FRHITextureViewDesc ViewDesc;
            ViewDesc.Texture                                       = *Texture;
            TExpected<TSharedRef<IRHITextureView>, FRHIError> View = Device.CreateTextureView( ViewDesc );
            LUMEN_CHECK_TRUE( View.HasValue() );
            Write.Resource = FRHITextureBinding{ .View = *View };
            LUMEN_CHECK_TRUE( Write.Validate().HasValue() );
            Write.Resource = FRHITextureBinding{};
            LUMEN_CHECK_HAS_ERROR( Write.Validate() );
            TExpected<TSharedRef<IRHISampler>, FRHIError> Sampler = Device.CreateSampler( {} );
            LUMEN_CHECK_TRUE( Sampler.HasValue() );
            Write.Resource = FRHISamplerBinding{ .Sampler = *Sampler };
            LUMEN_CHECK_TRUE( Write.Validate().HasValue() );
            Write.Resource = FRHICombinedTextureSamplerBinding{ .View = *View, .Sampler = *Sampler };
            LUMEN_CHECK_TRUE( Write.Validate().HasValue() );
            Write.Resource = FRHICombinedTextureSamplerBinding{ .View = *View, .Sampler = {} };
            LUMEN_CHECK_HAS_ERROR( Write.Validate() );
            Write.Resource = FRHICombinedTextureSamplerBinding{ .View = {}, .Sampler = *Sampler };
            LUMEN_CHECK_HAS_ERROR( Write.Validate() );
            Write.Resource = FRHISamplerBinding{};
            LUMEN_CHECK_HAS_ERROR( Write.Validate() );
        }

    } // namespace

} // namespace RHI

} // namespace Lumen
