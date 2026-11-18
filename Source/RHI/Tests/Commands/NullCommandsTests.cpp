/**
 * @file NullCommandsTests.cpp
 * @brief Tests Null recording history, scopes, and reset.
 */

#include "Commands/NullCommands.hpp"
#include "Pipelines/NullPipelines.hpp"
#include "Queries/NullQueries.hpp"
#include "Resources/NullResources.hpp"
#include <RHI/Utilities/RHIScopedDebugRegion.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    namespace
    {
        LUMEN_TEST( NullCommands, RecordsDrawScopeAndClearsHistoryOnReset )
        {
            TSharedRef<Tests::FNullCommandAllocator> Allocator =
                MakeShared<Tests::FNullCommandAllocator>( ERHIQueueType::Graphics, FRHIObjectId{ 1ULL } );
            Tests::FNullCommandList List( Allocator, FRHIObjectId{ 2ULL } );
            LUMEN_CHECK_HAS_ERROR( List.Draw( { .VertexCount = 3U } ) );
            LUMEN_CHECK_TRUE( List.Begin().HasValue() );
            FRHITextureDesc TextureDesc{ .Extent         = { 4U, 4U, 1U },
                                         .Format         = ERHIFormat::RGBA8Unorm,
                                         .Usage          = ERHIResourceUsage::RenderTarget,
                                         .OptimizedClear = {} };
            TSharedRef<Tests::FNullTexture> Texture =
                MakeShared<Tests::FNullTexture>( TextureDesc, FRHIResourceRequirements{}, FRHIObjectId{ 3ULL } );
            FRHITextureViewDesc ViewDesc;
            ViewDesc.Texture                         = StaticCastSharedRef<IRHITexture>( Texture );
            TSharedRef<Tests::FNullTextureView> View = MakeShared<Tests::FNullTextureView>( ViewDesc, FRHIObjectId{ 4ULL } );
            FRHIRenderingDesc Rendering;
            Rendering.Area = { 0, 0, 4U, 4U };
            Rendering.ColorAttachments.push_back(
                { .View = View, .LoadOp = ERHIRenderingLoadOp::Load, .StoreOp = ERHIRenderingStoreOp::Store, .ClearValue = {} } );
            LUMEN_CHECK_TRUE( List.BeginRendering( Rendering ).HasValue() );
            LUMEN_CHECK_HAS_ERROR( List.BeginRendering( Rendering ) );
            TSharedRef<Tests::FNullPipeline> Pipeline =
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
            const TExpected<FRHIDrawArguments, FRHIError> Draw =
                List.GetStream().ReadPayload<FRHIDrawArguments>( 3U, static_cast<UInt32>( Tests::ERHINullCommandOpcode::Draw ) );
            LUMEN_CHECK_TRUE( Draw.HasValue() );
            LUMEN_CHECK_EQ( Draw->VertexCount, 3U );
            LUMEN_CHECK_TRUE( List.Reset().HasValue() );
            LUMEN_CHECK_EQ( List.GetStream().GetCommandCount(), 0U );
            LUMEN_CHECK_EQ( List.GetStream().GetRetainedCount(), 0U );
        }

        LUMEN_TEST( NullCommands, BeginQueryRetainsPoolUntilReset )
        {
            TSharedRef<Tests::FNullCommandAllocator> Allocator =
                MakeShared<Tests::FNullCommandAllocator>( ERHIQueueType::Graphics, FRHIObjectId{ 1ULL } );
            Tests::FNullCommandList List( Allocator, FRHIObjectId{ 2ULL } );
            const TSharedRef<Tests::FNullQueryPool> Pool = MakeShared<Tests::FNullQueryPool>(
                FRHIQueryPoolDesc{}, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 3ULL } );
            const TSharedRef<IRHIQueryPool> QueryPool = StaticCastSharedRef<IRHIQueryPool>( Pool );
            LUMEN_CHECK_EQ( List.GetStream().GetRetainedCount(), 0U );
            LUMEN_CHECK_TRUE( List.Begin().HasValue() );
            LUMEN_CHECK_TRUE( List.BeginQuery( QueryPool, 0U ).HasValue() );
            LUMEN_CHECK_EQ( List.GetStream().GetRetainedCount(), 1U );
            LUMEN_CHECK_TRUE( List.EndQuery( QueryPool, 0U ).HasValue() );
            LUMEN_CHECK_EQ( List.GetStream().GetRetainedCount(), 1U );
            LUMEN_CHECK_TRUE( List.End().HasValue() );
            LUMEN_CHECK_TRUE( List.Reset().HasValue() );
            LUMEN_CHECK_EQ( List.GetStream().GetRetainedCount(), 0U );
        }

    } // namespace

} // namespace RHI

} // namespace Lumen
