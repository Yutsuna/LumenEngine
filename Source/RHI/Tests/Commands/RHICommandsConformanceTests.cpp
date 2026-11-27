/**
 * @file RHICommandsConformanceTests.cpp
 * @brief Runs portable record, rendering, and submit scenarios.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"
#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Pipelines/IRHIPipeline.hpp>
#include <RHI/Pipelines/RHIComputePipelineDesc.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        TSharedRef<IRHIPipeline> CreateGraphicsPipeline ( IRHIDevice &Device )
        {
            const TExpected<TSharedRef<IRHIPipelineLayout>, FRHIError> Layout = Device.CreatePipelineLayout( {} );
            LUMEN_CHECK_TRUE( Layout.HasValue() );
            const TExpected<TSharedRef<IRHIShader>, FRHIError> VertexShader =
                Tests::GetRHIConformanceDeviceFactory().CreateShader( Device, ERHIShaderStage::Vertex );
            const TExpected<TSharedRef<IRHIShader>, FRHIError> PixelShader =
                Tests::GetRHIConformanceDeviceFactory().CreateShader( Device, ERHIShaderStage::Pixel );
            LUMEN_CHECK_TRUE( VertexShader.HasValue() );
            LUMEN_CHECK_TRUE( PixelShader.HasValue() );
            FRHIGraphicsPipelineDesc Desc;
            Desc.VertexShader                                             = *VertexShader;
            Desc.PixelShader                                              = *PixelShader;
            Desc.Layout                                                   = *Layout;
            Desc.ColorFormats                                             = { ERHIFormat::RGBA8Unorm };
            const TExpected<TSharedRef<IRHIPipeline>, FRHIError> Pipeline = Device.CreateGraphicsPipeline( Desc );
            LUMEN_CHECK_TRUE( Pipeline.HasValue() );
            return *Pipeline;
        }

        LUMEN_TEST( RHICommandsConformance, RecordsCopiesAndSubmitsExecutableList )
        {
            const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
            TSharedRef<IRHIBuffer> Source =
                *( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource } );
            TSharedRef<IRHIBuffer> Destination =
                *( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopyDestination } );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            LUMEN_CHECK_TRUE( List->CopyBuffer( Source, Destination, { .Size = 64ULL } ).HasValue() );
            LUMEN_CHECK_TRUE( List->End().HasValue() );
            LUMEN_CHECK_TRUE( List->GetStream().GetRetainedCount() >= 2U );
            TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
            FRHISubmitDesc Submit;
            Submit.CommandLists = { List };
            Submit.Completion   = { 1ULL };
            LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
            LUMEN_CHECK_TRUE( Queue->Wait( { 1ULL }, 1.0 ).HasValue() );
            LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Executable );
            LUMEN_CHECK_TRUE( Allocator->Reset( Queue->GetCompletedValue() ).HasValue() );
            LUMEN_CHECK_EQ( List->GetStream().GetRetainedCount(), 0U );
        }

        LUMEN_TEST( RHICommandsConformance, RecordsDynamicRenderingAndRebindsCompatiblePipelines )
        {
            const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            if ( not( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::DynamicRendering ) )
            {
                return;
            }
            const TExpected<TSharedRef<IRHITexture>, FRHIError> Texture =
                ( *Device )->CreateTexture( { .Extent         = { 16U, 16U, 1U },
                                              .Format         = ERHIFormat::RGBA8Unorm,
                                              .Usage          = ERHIResourceUsage::RenderTarget,
                                              .OptimizedClear = {} } );
            LUMEN_CHECK_TRUE( Texture.HasValue() );
            FRHITextureViewDesc ViewDesc;
            ViewDesc.Texture                                             = *Texture;
            const TExpected<TSharedRef<IRHITextureView>, FRHIError> View = ( *Device )->CreateTextureView( ViewDesc );
            LUMEN_CHECK_TRUE( View.HasValue() );
            TSharedRef<IRHIPipeline> FirstPipeline  = CreateGraphicsPipeline( **Device );
            TSharedRef<IRHIPipeline> SecondPipeline = CreateGraphicsPipeline( **Device );
            LUMEN_CHECK_NOT_EQ( FirstPipeline->GetId().GetValue(), SecondPipeline->GetId().GetValue() );
            LUMEN_CHECK_TRUE( FirstPipeline->GetCompatibility() == SecondPipeline->GetCompatibility() );

            TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            FRHIRenderingDesc Rendering;
            Rendering.Area = { 0, 0, 16U, 16U };
            FRHIRenderingAttachment Attachment;
            Attachment.View = *View;
            Rendering.ColorAttachments.push_back( Attachment );
            LUMEN_CHECK_TRUE( List->BeginRendering( Rendering ).HasValue() );
            LUMEN_CHECK_TRUE( List->SetPipeline( FirstPipeline ).HasValue() );
            LUMEN_CHECK_TRUE( List->Draw( { .VertexCount = 3U } ).HasValue() );
            LUMEN_CHECK_TRUE( List->SetPipeline( SecondPipeline ).HasValue() );
            LUMEN_CHECK_TRUE( List->Draw( { .VertexCount = 3U, .FirstVertex = 3U } ).HasValue() );
            LUMEN_CHECK_TRUE( List->EndRendering().HasValue() );
            LUMEN_CHECK_TRUE( List->End().HasValue() );

            TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
            FRHISubmitDesc Submit;
            Submit.CommandLists = { List };
            Submit.Completion   = { 1ULL };
            LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
            LUMEN_CHECK_TRUE( Queue->WaitIdle().HasValue() );
        }

        LUMEN_TEST( RHICommandsConformance, RejectsInvalidLifecycleCopiesAndTransitions )
        {
            const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
            const FRHIExpected EarlyEnd                = List->End();
            LUMEN_CHECK_HAS_ERROR( EarlyEnd );
            LUMEN_CHECK_TRUE(
                Tests::HasRHIConformanceError( EarlyEnd.Error(), ERHIErrorCode::InvalidState, ERHIErrorCondition::Command ) );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            LUMEN_CHECK_HAS_ERROR( List->Draw( { .VertexCount = 3U } ) );

            TSharedRef<IRHIBuffer> Source =
                *( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource } );
            TSharedRef<IRHIBuffer> Destination =
                *( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopyDestination } );
            const FRHIExpected EmptyCopy = List->CopyBuffer( Source, Destination, {} );
            LUMEN_CHECK_HAS_ERROR( EmptyCopy );
            LUMEN_CHECK_EQ( EmptyCopy.Error().Code, ERHIErrorCode::InvalidArgument );
            LUMEN_CHECK_EQ( EmptyCopy.Error().Condition, ERHIErrorCondition::Command );

            FRHIBufferTransition BufferTransition;
            BufferTransition.Buffer                = Source;
            BufferTransition.Before                = ERHIResourceState::Common;
            BufferTransition.After                 = ERHIResourceState::Common;
            const FRHITransition InvalidTransition = BufferTransition;
            LUMEN_CHECK_HAS_ERROR( List->Transition( TSpan<const FRHITransition>( &InvalidTransition, 1U ) ) );
            BufferTransition.Before              = ERHIResourceState::Common;
            BufferTransition.After               = ERHIResourceState::CopySource;
            const FRHITransition ValidTransition = BufferTransition;
            LUMEN_CHECK_TRUE( List->Transition( TSpan<const FRHITransition>( &ValidTransition, 1U ) ).HasValue() );
            LUMEN_CHECK_TRUE( List->End().HasValue() );
            LUMEN_CHECK_HAS_ERROR( List->Begin() );
        }

        LUMEN_TEST( RHICommandsConformance, RecordsInlineConstantsThroughValidation )
        {
            const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device =
                Tests::GetRHIConformanceDeviceFactory().CreateDevice( true );
            LUMEN_CHECK_TRUE( Device.HasValue() );
            FRHIPipelineLayoutDesc LayoutDesc;
            LayoutDesc.InlineConstants.push_back( { .Offset = 0U, .Size = 4U, .Stages = ERHIShaderStage::Compute } );
            const TExpected<TSharedRef<IRHIPipelineLayout>, FRHIError> Layout = ( *Device )->CreatePipelineLayout( LayoutDesc );
            const TExpected<TSharedRef<IRHIShader>, FRHIError> Shader =
                Tests::GetRHIConformanceDeviceFactory().CreateShader( **Device, ERHIShaderStage::Compute );
            LUMEN_CHECK_TRUE( Layout.HasValue() );
            LUMEN_CHECK_TRUE( Shader.HasValue() );
            FRHIComputePipelineDesc PipelineDesc;
            PipelineDesc.Shader                                           = *Shader;
            PipelineDesc.Layout                                           = *Layout;
            const TExpected<TSharedRef<IRHIPipeline>, FRHIError> Pipeline = ( *Device )->CreateComputePipeline( PipelineDesc );
            LUMEN_CHECK_TRUE( Pipeline.HasValue() );

            TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
            const TArray<Byte, 4U> Values              = { Byte{ 4U }, Byte{ 3U }, Byte{ 2U }, Byte{ 1U } };
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            LUMEN_CHECK_TRUE( List->SetPipeline( *Pipeline ).HasValue() );
            LUMEN_CHECK_HAS_ERROR( List->SetInlineConstants( ERHIShaderStage::Compute, 2U, Values ) );
            LUMEN_CHECK_TRUE( List->SetInlineConstants( ERHIShaderStage::Compute, 0U, Values ).HasValue() );
            LUMEN_CHECK_TRUE( List->End().HasValue() );
            LUMEN_CHECK_EQ( List->GetStream().GetCommandCount(), 2U );
        }

        LUMEN_TEST( RHICommandsConformance, RecordsIndirectDispatchWhenSupported )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            if ( not( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::IndirectDraw ) )
            {
                return;
            }
            const TExpected<Bool, FRHIError> HasCompute = Tests::HasRHIConformanceQueueFamily( ERHIQueueType::Compute );
            LUMEN_CHECK_TRUE( HasCompute.HasValue() );
            if ( not *HasCompute )
            {
                return;
            }
            TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Compute );
            TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
            TSharedRef<IRHIPipelineLayout> Layout      = *( *Device )->CreatePipelineLayout( {} );
            TSharedRef<IRHIShader> Shader =
                *Tests::GetRHIConformanceDeviceFactory().CreateShader( **Device, ERHIShaderStage::Compute );
            TSharedRef<IRHIPipeline> Pipeline = *( *Device )->CreateComputePipeline( { .Shader = Shader, .Layout = Layout } );
            TSharedRef<IRHIBuffer> Arguments =
                *( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::IndirectArgument } );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            LUMEN_CHECK_TRUE( List->SetPipeline( Pipeline ).HasValue() );
            LUMEN_CHECK_TRUE( List->DispatchIndirect( Arguments, { .Count = 2U, .Stride = 16U } ).HasValue() );
            LUMEN_CHECK_HAS_ERROR( List->BeginRendering( {} ) );
            LUMEN_CHECK_HAS_ERROR( List->EndRendering() );
            LUMEN_CHECK_TRUE( List->End().HasValue() );
            LUMEN_CHECK_HAS_ERROR( List->Dispatch( {} ) );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
