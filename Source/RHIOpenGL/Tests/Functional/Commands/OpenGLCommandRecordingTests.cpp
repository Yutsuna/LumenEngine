/**
 * @file OpenGLCommandRecordingTests.cpp
 * @brief Functional coverage for command paths wired in M4.1.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"

#include <HAL/PlatformTime.hpp>

#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Pipelines/IRHIPipeline.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/IRHITextureView.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    using namespace RHI;

    TSharedRef<IRHIPipeline> MakePipeline ( IRHIDevice &InDevice, const ERHIDynamicState InDynamicStates )
    {
        const TExpected<TSharedRef<IRHIPipelineLayout>, FRHIError> Layout = InDevice.CreatePipelineLayout( {} );
        LUMEN_CHECK_TRUE( Layout.HasValue() );
        const TExpected<TSharedRef<IRHIShader>, FRHIError> Vertex =
            RHI::Tests::GetRHIConformanceDeviceFactory().CreateShader( InDevice, ERHIShaderStage::Vertex );
        const TExpected<TSharedRef<IRHIShader>, FRHIError> Pixel =
            RHI::Tests::GetRHIConformanceDeviceFactory().CreateShader( InDevice, ERHIShaderStage::Pixel );
        LUMEN_CHECK_TRUE( Vertex.HasValue() );
        LUMEN_CHECK_TRUE( Pixel.HasValue() );
        FRHIGraphicsPipelineDesc Desc;
        Desc.VertexShader  = *Vertex;
        Desc.PixelShader   = *Pixel;
        Desc.Layout        = *Layout;
        Desc.ColorFormats  = { ERHIFormat::RGBA8Unorm };
        Desc.DynamicStates = InDynamicStates;
        Desc.VertexLayout.Buffers.push_back( { .Slot = 0U, .Stride = 12U, .Rate = ERHIVertexInputRate::Vertex } );
        const TExpected<TSharedRef<IRHIPipeline>, FRHIError> Pipeline = InDevice.CreateGraphicsPipeline( Desc );
        LUMEN_CHECK_TRUE( Pipeline.HasValue() );
        return *Pipeline;
    }

    LUMEN_TEST( OpenGLCommandRecording, RecordsAndReplaysEveryWiredCommand )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );

        const TExpected<TSharedRef<IRHITexture>, FRHIError> Color = ( *Device )->CreateTexture(
            { .Extent = { 16U, 16U, 1U },
              .Format = ERHIFormat::RGBA8Unorm,
              .Usage  = ERHIResourceUsage::RenderTarget | ERHIResourceUsage::CopySource | ERHIResourceUsage::CopyDestination,
              .OptimizedClear = {} } );
        const TExpected<TSharedRef<IRHITexture>, FRHIError> Staging =
            ( *Device )->CreateTexture( { .Extent         = { 16U, 16U, 1U },
                                          .Format         = ERHIFormat::RGBA8Unorm,
                                          .Usage          = ERHIResourceUsage::CopySource | ERHIResourceUsage::CopyDestination,
                                          .OptimizedClear = {} } );
        LUMEN_CHECK_TRUE( Color.HasValue() );
        LUMEN_CHECK_TRUE( Staging.HasValue() );
        FRHITextureViewDesc ViewDesc;
        ViewDesc.Texture                                             = *Color;
        const TExpected<TSharedRef<IRHITextureView>, FRHIError> View = ( *Device )->CreateTextureView( ViewDesc );
        LUMEN_CHECK_TRUE( View.HasValue() );

        const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Transfer = ( *Device )->CreateBuffer(
            { .Size = 4096ULL, .Usage = ERHIResourceUsage::CopySource | ERHIResourceUsage::CopyDestination } );
        const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Vertices =
            ( *Device )->CreateBuffer( { .Size = 4096ULL, .Usage = ERHIResourceUsage::VertexBuffer } );
        const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Indices =
            ( *Device )->CreateBuffer( { .Size = 4096ULL, .Usage = ERHIResourceUsage::IndexBuffer } );
        const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Indirect =
            ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::IndirectArgument } );
        LUMEN_CHECK_TRUE( Transfer.HasValue() );
        LUMEN_CHECK_TRUE( Vertices.HasValue() );
        LUMEN_CHECK_TRUE( Indices.HasValue() );
        LUMEN_CHECK_TRUE( Indirect.HasValue() );

        TSharedRef<IRHIPipeline> Pipeline =
            MakePipeline( **Device, ERHIDynamicState::Viewport | ERHIDynamicState::Scissor | ERHIDynamicState::BlendConstants |
                                        ERHIDynamicState::DepthBias | ERHIDynamicState::StencilReference );
        TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
        TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        FRHITextureCopy TextureCopy;
        TextureCopy.Extent = { 16U, 16U, 1U };
        FRHIBufferTextureCopy BufferTextureCopy;
        BufferTextureCopy.RowPitch = 64ULL;
        BufferTextureCopy.Extent   = { 16U, 16U, 1U };
        LUMEN_CHECK_TRUE( List->CopyTexture( *Staging, *Color, TextureCopy ).HasValue() );
        LUMEN_CHECK_TRUE( List->CopyBufferToTexture( *Transfer, *Color, BufferTextureCopy ).HasValue() );
        LUMEN_CHECK_TRUE( List->CopyTextureToBuffer( *Color, *Transfer, BufferTextureCopy ).HasValue() );
        LUMEN_CHECK_TRUE( List->SetViewport( { .Width = 16.0F, .Height = 16.0F } ).HasValue() );
        LUMEN_CHECK_TRUE( List->SetScissor( { 0, 0, 16U, 16U } ).HasValue() );
        LUMEN_CHECK_TRUE( List->SetPipeline( Pipeline ).HasValue() );
        LUMEN_CHECK_TRUE( List->SetBlendConstants( { 0.25F, 0.5F, 0.75F, 1.0F } ).HasValue() );
        LUMEN_CHECK_TRUE( List->SetDepthBias( 1.0F, 2.0F ).HasValue() );
        LUMEN_CHECK_TRUE( List->SetStencilReference( 7U ).HasValue() );
        LUMEN_CHECK_TRUE( List->SetVertexBuffer( 0U, *Vertices, 0ULL ).HasValue() );
        LUMEN_CHECK_TRUE( List->SetIndexBuffer( *Indices, 0ULL, ERHIIndexFormat::Index16 ).HasValue() );

        FRHIRenderingDesc Rendering;
        Rendering.Area = { 0, 0, 16U, 16U };
        FRHIRenderingAttachment Attachment;
        Attachment.View = *View;
        Rendering.ColorAttachments.push_back( Attachment );
        LUMEN_CHECK_TRUE( List->BeginRendering( Rendering ).HasValue() );
        LUMEN_CHECK_TRUE( List->DrawIndexed( { .IndexCount = 3U } ).HasValue() );
        FRHIIndirectArguments IndirectDesc;
        IndirectDesc.Stride = 16U;
        LUMEN_CHECK_TRUE( List->DrawIndirect( *Indirect, IndirectDesc ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndRendering().HasValue() );
        LUMEN_CHECK_TRUE( List->BeginDebugRegion( "M4.1 replay" ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndDebugRegion().HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        LUMEN_CHECK_TRUE( List->GetStream().GetRetainedCount() >= 6U );

        TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Completion   = { 1ULL };
        LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
        LUMEN_CHECK_TRUE( Queue->WaitIdle().HasValue() );
        /** NOTE: WaitIdle observes the timeline before the worker destroys the retired packet. */
        HAL::FPlatformTime::Sleep( 0.01 );
        LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Executable );
    }

    LUMEN_TEST( OpenGLCommandRecording, DynamicStateRequiresPipelineDeclaration )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TSharedRef<IRHIPipeline> Pipeline          = MakePipeline( **Device, ERHIDynamicState::None );
        TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
        TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->SetPipeline( Pipeline ).HasValue() );
        const Float32 Constants[4] = { 0.0F, 0.0F, 0.0F, 1.0F };
        LUMEN_CHECK_HAS_ERROR( List->SetBlendConstants( Constants ) );
        LUMEN_CHECK_HAS_ERROR( List->SetDepthBias( 1.0F, 1.0F ) );
        LUMEN_CHECK_HAS_ERROR( List->SetStencilReference( 1U ) );
    }

} // namespace

} // namespace Lumen
