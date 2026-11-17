/**
 * @file RHIValidationCommandListTests.cpp
 * @brief Tests command lifecycle, rendering, pipeline, and debug-scope validation.
 */
#include "Validation/RHIValidationReporter.hpp"
#include "Validation/RHIValidationTestUtils.hpp"
#include <Commands/IRHICommandAllocator.hpp>
#include <Commands/IRHICommandList.hpp>
#include <Pipelines/NullPipelines.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIValidationCommandList, RejectsIllegalLifecycleAndUnclosedDebugScope )
    {
        Tests::FRecordingDebugSink Sink;
        auto Device    = Tests::Validation::MakeDevice( Sink );
        auto Allocator = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
        auto List      = *Device->CreateCommandList( Allocator );
        LUMEN_CHECK_EQ( List->End().Error().Code, ERHIErrorCode::ValidationFailed );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        const FRHIDrawArguments DrawArguments{ .VertexCount = 3U };
        LUMEN_CHECK_EQ( List->Draw( DrawArguments ).Error().Code, ERHIErrorCode::ValidationFailed );
        LUMEN_CHECK_TRUE( List->BeginDebugRegion( "frame" ).HasValue() );
        LUMEN_CHECK_EQ( List->End().Error().Code, ERHIErrorCode::ValidationFailed );
        LUMEN_CHECK_TRUE( List->EndDebugRegion().HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        LUMEN_CHECK_EQ( List->Begin().Error().Code, ERHIErrorCode::ValidationFailed );
        LUMEN_CHECK_TRUE( Sink.ContainsCode( static_cast<UInt32>( ERHIValidationMessageCode::InvalidCommandState ) ) );
    }

    LUMEN_TEST( RHIValidationCommandList, ValidatesRenderingPipelineAndForwardsUnwrappedViews )
    {
        Tests::FRecordingDebugSink Sink;
        auto Device  = Tests::Validation::MakeDevice( Sink );
        auto Texture = Tests::Validation::MakeTexture( Device );
        FRHITextureViewDesc ViewDesc;
        ViewDesc.Texture = Texture;
        auto View        = *Device->CreateTextureView( ViewDesc );
        auto Allocator   = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
        auto List        = *Device->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        FRHIRenderingDesc Rendering;
        Rendering.Area = {
            .Position = {
                .X = 0,
                .Y = 0,
            },
            .Size = {
                .Width = 16U,
                .Height = 16U,
            },
        };
        FRHIRenderingAttachment Attachment;
        Attachment.View = View;
        Rendering.ColorAttachments.push_back( Attachment );
        LUMEN_CHECK_TRUE( List->BeginRendering( Rendering ).HasValue() );
        const FRHIDrawArguments DrawArguments{ .VertexCount = 3U };
        LUMEN_CHECK_EQ( List->Draw( DrawArguments ).Error().Code, ERHIErrorCode::ValidationFailed );
        auto Pipeline =
            MakeShared<Tests::FNullPipeline>( ERHIPipelineType::Graphics, FRHIPipelineCompatibility{}, FRHIObjectId{ 10000ULL } );
        LUMEN_CHECK_TRUE( List->SetPipeline( Pipeline ).HasValue() );
        LUMEN_CHECK_TRUE( List->Draw( DrawArguments ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndRendering().HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        LUMEN_CHECK_EQ( View->GetTexture().Get(), Texture.Get() );
        LUMEN_CHECK_TRUE( Allocator->Reset( FRHITimelineValue{} ).HasValue() );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->BeginRendering( Rendering ).HasValue() );
        LUMEN_CHECK_EQ( List->Draw( DrawArguments ).Error().Code, ERHIErrorCode::ValidationFailed );
        LUMEN_CHECK_TRUE( List->EndRendering().HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
    }
} // namespace
} // namespace Lumen
