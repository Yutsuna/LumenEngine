/**
 * @file NullCommandsTests.cpp
 * @brief Tests Null recording history, scopes, and reset.
 */

#include "Commands/NullCommands.hpp"
#include "Pipelines/NullPipelines.hpp"
#include "Queries/NullQueries.hpp"
#include "Resources/NullResources.hpp"
#include <RHI/Pipelines/RHIInlineConstantRange.hpp>
#include <RHI/Utilities/RHIScopedDebugRegion.hpp>
#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>
#include <cstring>

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

        LUMEN_TEST( NullCommands, CopiesInlineConstantBytesIntoTheCommandStream )
        {
            TSharedRef<Tests::FNullCommandAllocator> Allocator =
                MakeShared<Tests::FNullCommandAllocator>( ERHIQueueType::Compute, FRHIObjectId{ 1ULL } );
            Tests::FNullCommandList List( Allocator, FRHIObjectId{ 2ULL } );
            TSharedRef<Tests::FNullPipeline> Pipeline =
                MakeShared<Tests::FNullPipeline>( ERHIPipelineType::Compute, FRHIPipelineCompatibility{}, FRHIObjectId{ 3ULL } );
            LUMEN_CHECK_TRUE( List.Begin().HasValue() );
            LUMEN_CHECK_TRUE( List.SetPipeline( Pipeline ).HasValue() );

            TArray<Byte, 4U> Values = { Byte{ 1U }, Byte{ 2U }, Byte{ 3U }, Byte{ 4U } };
            LUMEN_CHECK_TRUE( List.SetInlineConstants( ERHIShaderStage::Compute, 0U, Values ).HasValue() );
            Values[0] = Byte{ 9U };
            LUMEN_CHECK_HAS_ERROR( List.SetInlineConstants( ERHIShaderStage::Compute, 2U, Values ) );
            LUMEN_CHECK_HAS_ERROR( List.SetInlineConstants( ERHIShaderStage::Compute, 0U, {} ) );

            const TExpected<FRHICommandView, FRHIError> Command = List.GetStream().Read( 1U );
            LUMEN_CHECK_TRUE( Command.HasValue() );
            LUMEN_CHECK_EQ( Command->Opcode, static_cast<UInt32>( Tests::ERHINullCommandOpcode::SetInlineConstants ) );
            LUMEN_CHECK_EQ( Command->Payload.size(), sizeof( FRHIInlineConstantCommand ) + 4U );
            FRHIInlineConstantCommand Record;
            std::memcpy( std::addressof( Record ), Command->Payload.data(), sizeof( Record ) );
            LUMEN_CHECK_EQ( Record.Stages, ERHIShaderStage::Compute );
            LUMEN_CHECK_EQ( Record.Offset, 0U );
            LUMEN_CHECK_EQ( Record.Size, 4U );
            LUMEN_CHECK_EQ( static_cast<UInt32>( Command->Payload[sizeof( Record )] ), 1U );
            LUMEN_CHECK_TRUE( List.End().HasValue() );
        }

    } // namespace

} // namespace RHI

} // namespace Lumen
