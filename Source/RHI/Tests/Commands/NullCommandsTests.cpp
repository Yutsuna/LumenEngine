/**
 * @file NullCommandsTests.cpp
 * @brief Tests Null recording history, scopes, and reset.
 */

#include "Commands/NullCommands.hpp"
#include "Pipelines/NullPipelines.hpp"
#include "Resources/NullResources.hpp"
#include <Tests/UnitTest.hpp>
#include <Utilities/RHIScopedDebugRegion.hpp>

namespace Lumen
{

namespace
{
    LUMEN_TEST( NullCommands, RecordsDrawScopeAndClearsHistoryOnReset )
    {
        auto Allocator = MakeShared<Tests::FNullCommandAllocator>( ERHIQueueType::Graphics, FRHIObjectId{ 1ULL } );
        Tests::FNullCommandList List( Allocator, FRHIObjectId{ 2ULL } );
        LUMEN_CHECK_HAS_ERROR( List.Draw( { .VertexCount = 3U } ) );
        LUMEN_CHECK_TRUE( List.Begin().HasValue() );
        FRHITextureDesc TextureDesc{ .Extent         = { .Width = 4U, .Height = 4U, .Depth = 1U },
                                     .Format         = ERHIFormat::RGBA8Unorm,
                                     .Usage          = ERHIResourceUsage::RenderTarget,
                                     .OptimizedClear = {} };
        auto Texture = MakeShared<Tests::FNullTexture>( TextureDesc, FRHIResourceRequirements{}, FRHIObjectId{ 3ULL } );
        FRHITextureViewDesc ViewDesc;
        ViewDesc.Texture = StaticCastSharedRef<IRHITexture>( Texture );
        auto View        = MakeShared<Tests::FNullTextureView>( ViewDesc, FRHIObjectId{ 4ULL } );
        FRHIRenderingDesc Rendering;
        Rendering.Area = { .X = 0, .Y = 0, .Width = 4U, .Height = 4U };
        Rendering.ColorAttachments.push_back(
            { .View = View, .LoadOp = ERHIRenderingLoadOp::Load, .StoreOp = ERHIRenderingStoreOp::Store, .ClearValue = {} } );
        LUMEN_CHECK_TRUE( List.BeginRendering( Rendering ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( List.BeginRendering( Rendering ) );
        auto Pipeline =
            MakeShared<Tests::FNullPipeline>( ERHIPipelineType::Graphics, FRHIPipelineCompatibility{}, FRHIObjectId{ 5ULL } );
        LUMEN_CHECK_TRUE( List.SetPipeline( Pipeline ).HasValue() );
        {
            FRHIScopedDebugRegion Region( List, "Draw" );
            LUMEN_CHECK_TRUE( List.Draw( { .VertexCount = 3U } ).HasValue() );
        }
        LUMEN_CHECK_TRUE( List.EndRendering().HasValue() );
        LUMEN_CHECK_TRUE( List.End().HasValue() );
        LUMEN_CHECK_EQ( List.GetState(), ERHICommandListState::Executable );
        LUMEN_CHECK_EQ( List.GetStream().GetCommandCount(), 6U );
        LUMEN_CHECK_TRUE( List.GetStream().GetRetainedCount() >= 2U );
        auto Draw =
            List.GetStream().ReadPayload<FRHIDrawArguments>( 3U, static_cast<UInt32>( Tests::ERHINullCommandOpcode::Draw ) );
        LUMEN_CHECK_TRUE( Draw.HasValue() );
        LUMEN_CHECK_EQ( Draw->VertexCount, 3U );
        LUMEN_CHECK_TRUE( List.Reset().HasValue() );
        LUMEN_CHECK_EQ( List.GetStream().GetCommandCount(), 0U );
        LUMEN_CHECK_EQ( List.GetStream().GetRetainedCount(), 0U );
    }

} // namespace

} // namespace Lumen
