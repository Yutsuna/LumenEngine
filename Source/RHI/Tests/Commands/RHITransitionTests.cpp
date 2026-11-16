/**
 * @file RHITransitionTests.cpp
 * @brief Tests buffer and subresource barriers.
 */

#include "Device/NullDevice.hpp"
#include <Commands/RHITransition.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    LUMEN_TEST( RHITransition, ValidatesStatesResourcesAndSubresources )
    {
        Tests::FNullDevice Device;
        auto Buffer = *Device.CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource } );
        FRHIBufferTransition BufferTransition{
            .Buffer = Buffer, .Before = ERHIResourceState::Common, .After = ERHIResourceState::CopySource };
        LUMEN_CHECK_TRUE( BufferTransition.Validate().HasValue() );
        BufferTransition.After = ERHIResourceState::Common;
        LUMEN_CHECK_HAS_ERROR( BufferTransition.Validate() );
        BufferTransition.After = ERHIResourceState::CopySource;
        FRHITransition Variant = BufferTransition;
        LUMEN_CHECK_TRUE( ValidateRHITransition( Variant ).HasValue() );
        auto Texture = *Device.CreateTexture( { .Extent         = { .Width = 4U, .Height = 4U, .Depth = 1U },
                                                .Format         = ERHIFormat::RGBA8Unorm,
                                                .MipCount       = 2U,
                                                .Usage          = ERHIResourceUsage::CopySource,
                                                .OptimizedClear = {} } );
        FRHITextureTransition TextureTransition{
            .Texture = Texture, .Range = {}, .Before = ERHIResourceState::Common, .After = ERHIResourceState::CopySource };
        LUMEN_CHECK_TRUE( TextureTransition.Validate().HasValue() );
        TextureTransition.Range.BaseMip = 2U;
        LUMEN_CHECK_HAS_ERROR( TextureTransition.Validate() );
        TextureTransition.Range.BaseMip = 0U;
        TextureTransition.SourceQueue   = ERHIQueueType::Count;
        LUMEN_CHECK_HAS_ERROR( TextureTransition.Validate() );
    }

} // namespace

} // namespace Lumen
