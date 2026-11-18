/**
 * @file RHICommandsConformanceTests.cpp
 * @brief Runs portable record, rendering, and submit scenarios.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"
#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Pipelines/IRHIPipeline.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
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
            LUMEN_CHECK_TRUE( Queue->Wait( { 1ULL }, 0.0 ).HasValue() );
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
            const TExpected<void, FRHIError> EarlyEnd  = List->End();
            LUMEN_CHECK_HAS_ERROR( EarlyEnd );
            LUMEN_CHECK_EQ( EarlyEnd.Error().Code, ERHIErrorCode::InvalidState );
            LUMEN_CHECK_EQ( EarlyEnd.Error().Condition, ERHIErrorCondition::Command );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            LUMEN_CHECK_HAS_ERROR( List->Draw( { .VertexCount = 3U } ) );

            TSharedRef<IRHIBuffer> Source =
                *( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource } );
            TSharedRef<IRHIBuffer> Destination =
                *( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopyDestination } );
            const TExpected<void, FRHIError> EmptyCopy = List->CopyBuffer( Source, Destination, {} );
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

    } // namespace
} // namespace RHI

} // namespace Lumen
