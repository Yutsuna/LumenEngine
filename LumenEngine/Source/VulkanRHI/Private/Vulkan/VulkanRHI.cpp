/**
 * @file VulkanRHI.cpp
 * @brief Implementation of the Vulkan Render Hardware Interface.
 */

#include "Vulkan/VulkanRHI.hpp"
#include "Generic/GenericWindow.hpp"
#include "Vulkan/VulkanCore.hpp"

LumenEngine::VulkanRHI::FVulkanRHI::FVulkanRHI () noexcept : CommandListImpl( this )
{
    /* Empty */
}

void LumenEngine::VulkanRHI::FVulkanRHI::Initialize ( const LumenEngine::TSharedPtr<LumenEngine::FGenericWindow> &InWindow )
{
    if ( bIsInitialized )
    {
        return;
    }

    InitializeVulkanInstance( InWindow );
    InitializeVulkanDevice();
    Memory.Initialize( Instance.GetHandle(), PhysicalDevice.GetHandle(), LogicalDevice.GetHandle() );
    InitializeSwapChain( InWindow );
    FrameContext.Initialize( LogicalDevice.GetHandle(), LogicalDevice.GetGraphicsQueueFamily() );

    bIsInitialized = true;

    LUMEN_LOG_INFO( LogVulkanRHI, "Vulkan RHI initialized successfully." );
}

void LumenEngine::VulkanRHI::FVulkanRHI::Shutdown ()
{
    if ( not bIsInitialized )
    {
        return;
    }

    LogicalDevice.WaitIdle();

    PipelineRegistry.ForEach( [this] ( FVulkanPipeline &Pipeline ) { Pipeline.Cleanup( LogicalDevice.GetHandle() ); } );
    PipelineRegistry.Clear();

    MeshRegistry.ForEach( [this] ( FVulkanMesh &Mesh ) { Mesh.Cleanup( Memory.GetAllocator() ); } );
    MeshRegistry.Clear();

    FrameContext.Shutdown( LogicalDevice.GetHandle() );

    DestroySwapChain();

    Memory.Shutdown( LogicalDevice.GetHandle() );

    DestroyVulkanDevice();
    DestroyVulkanInstance();

    bIsInitialized = false;
    LUMEN_LOG_INFO( LogVulkanRHI, "Vulkan RHI shut down." );
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyVulkanInstance () noexcept
{
    Instance.Cleanup();
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyVulkanDevice () noexcept
{
    LogicalDevice.Cleanup();
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroySwapChain () noexcept
{
    SwapChain.Cleanup( LogicalDevice.GetHandle() );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeVulkanInstance ( const LumenEngine::TSharedPtr<LumenEngine::FGenericWindow> &InWindow )
{
    Instance.Initialize( InWindow );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeVulkanDevice ()
{
    const LumenEngine::TVector<const LumenEngine::AnsiChar *> DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    PhysicalDevice.Initialize( Instance.GetHandle(), Instance.GetSurface() );
    LogicalDevice.Initialize( PhysicalDevice.GetHandle(), Instance.GetSurface(), DeviceExtensions );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeSwapChain ( const LumenEngine::TSharedPtr<LumenEngine::FGenericWindow> &InWindow )
{
    const VkPhysicalDevice &PhysicalDeviceHandle = PhysicalDevice.GetHandle();
    const VkDevice &LogicalDeviceHandle          = LogicalDevice.GetHandle();
    const LumenEngine::Maths::FVec2u WindowSize  = static_cast<LumenEngine::Maths::FVec2u>( InWindow->GetWindowSize() );

    SwapChain.InitializeSynStructures( LogicalDeviceHandle );
    SwapChain.Create( PhysicalDeviceHandle, LogicalDeviceHandle, Instance.GetSurface(), VK_FORMAT_B8G8R8A8_SRGB, WindowSize, true );
}

LumenEngine::VulkanRHI::FVulkanLogicalDevice LumenEngine::VulkanRHI::FVulkanRHI::GetLogicalDevice () const noexcept
{
    return LogicalDevice;
}

LumenEngine::VulkanRHI::FVulkanPhysicalDevice LumenEngine::VulkanRHI::FVulkanRHI::GetPhysicalDevice () const noexcept
{
    return PhysicalDevice;
}

LumenEngine::VulkanRHI::FVulkanInstance LumenEngine::VulkanRHI::FVulkanRHI::GetInstance () const noexcept
{
    return Instance;
}

VmaAllocator LumenEngine::VulkanRHI::FVulkanRHI::GetAllocator () const noexcept
{
    return Memory.GetAllocator();
}

LumenEngine::VulkanRHI::FVulkanSwapChain &LumenEngine::VulkanRHI::FVulkanRHI::GetSwapChain () noexcept
{
    return SwapChain;
}

LumenEngine::RHI::IRHICommandList &LumenEngine::VulkanRHI::FVulkanRHI::GetCommandList () noexcept
{
    return CommandListImpl;
}

void LumenEngine::VulkanRHI::FVulkanRHI::WaitIdle () const noexcept
{
    LogicalDevice.WaitIdle();
}

LumenEngine::Bool LumenEngine::VulkanRHI::FVulkanRHI::BeginFrame ( const RHI::FGlobalUniformData &InUniforms )
{
    if ( not FrameContext.BeginFrame( LogicalDevice.GetHandle(), SwapChain ) )
    {
        return false;
    }

    const UInt32 CurrentFrame = FrameContext.GetCurrentFrameIndex();

    Memory.UpdateGlobalUniformData( CurrentFrame, InUniforms );

    /** Inform the CommandList which command buffer to use for this frame */
    CommandListImpl.SetActiveCommandBuffer( FrameContext.GetCurrentCommandBuffer().GetHandle() );

    return true;
}

void LumenEngine::VulkanRHI::FVulkanRHI::BeginRenderingInternal ( VkCommandBuffer InCmd, const LumenEngine::Float32 InClearColor[4] ) noexcept
{
    const VkImage &Image = SwapChain.GetImages()[FrameContext.GetCurrentImageIndex()];

    /** Transition swapchain image to color attachment layout for dynamic rendering */
    FrameContext.GetCurrentCommandBuffer().TransitionImageLayout( Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT );

    VkRenderingAttachmentInfo ColorAttachment{};
    ColorAttachment.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    ColorAttachment.imageView        = SwapChain.GetImageView( FrameContext.GetCurrentImageIndex() );
    ColorAttachment.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ColorAttachment.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorAttachment.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachment.clearValue.color = { { InClearColor[0], InClearColor[1], InClearColor[2], InClearColor[3] } };

    VkRenderingInfo RenderInfo{};
    RenderInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    RenderInfo.renderArea.extent    = SwapChain.GetExtent();
    RenderInfo.layerCount           = 1;
    RenderInfo.colorAttachmentCount = 1;
    RenderInfo.pColorAttachments    = &ColorAttachment;

    vkCmdBeginRendering( InCmd, &RenderInfo );

    /** Set dynamic viewport and scissor */
    const VkViewport Viewport{
        .x        = 0.F,
        .y        = 0.F,
        .width    = static_cast<LumenEngine::Float32>( RenderInfo.renderArea.extent.width ),
        .height   = static_cast<LumenEngine::Float32>( RenderInfo.renderArea.extent.height ),
        .minDepth = 0.F,
        .maxDepth = 1.F,
    };
    const VkRect2D Scissor{
        .offset = VkOffset2D{ .x = 0, .y = 0 },
        .extent = RenderInfo.renderArea.extent,
    };

    vkCmdSetViewport( InCmd, 0, 1, &Viewport );
    vkCmdSetScissor( InCmd, 0, 1, &Scissor );
}

void LumenEngine::VulkanRHI::FVulkanRHI::BindPipelineInternal ( VkCommandBuffer InCmd, const LumenEngine::RHI::FPipelineHandle InPipeline ) noexcept
{
    FVulkanPipeline *Pipeline = PipelineRegistry.Get( InPipeline );

    if ( Pipeline == nullptr )
    {
        return;
    }

    Pipeline->Bind( InCmd );

    const UInt32 CurrentFrame           = FrameContext.GetCurrentFrameIndex();
    VkDescriptorSet GlobalDescriptorSet = Memory.GetGlobalDescriptorSet( CurrentFrame );

    vkCmdBindDescriptorSets( InCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline->GetLayout(), 0, 1, &GlobalDescriptorSet, 0, nullptr );
}

void LumenEngine::VulkanRHI::FVulkanRHI::PushConstantsInternal (
    VkCommandBuffer InCmd, const LumenEngine::RHI::FPipelineHandle InPipeline, const void *InData, UInt32 InSize, UInt32 InOffset ) noexcept
{
    FVulkanPipeline *Pipeline = PipelineRegistry.Get( InPipeline );

    if ( Pipeline == nullptr )
    {
        return;
    }

    vkCmdPushConstants( InCmd, Pipeline->GetLayout(), VK_SHADER_STAGE_ALL_GRAPHICS, InOffset, InSize, InData );
}

void LumenEngine::VulkanRHI::FVulkanRHI::DrawMeshInternal ( VkCommandBuffer InCmd, const LumenEngine::RHI::FMeshHandle InMesh ) noexcept
{
    FVulkanMesh *Mesh = MeshRegistry.Get( InMesh );

    if ( Mesh == nullptr )
    {
        return;
    }

    Mesh->BindAndDraw( InCmd );
}

void LumenEngine::VulkanRHI::FVulkanRHI::EndFrame ()
{
    const VkImage &Image = SwapChain.GetImages()[FrameContext.GetCurrentImageIndex()];

    FrameContext.GetCurrentCommandBuffer().TransitionImageLayout( Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                                  VK_IMAGE_ASPECT_COLOR_BIT );

    FrameContext.SubmitAndPresent( SwapChain, LogicalDevice.GetGraphicsQueue() );
}

LumenEngine::RHI::FMeshHandle LumenEngine::VulkanRHI::FVulkanRHI::CreateMesh ( const LumenEngine::TVector<LumenEngine::Maths::FVertex> &InVertices,
                                                                               const LumenEngine::TVector<LumenEngine::UInt32> &InIndices )
{
    LumenEngine::VulkanRHI::FVulkanMesh NewMesh;

    NewMesh.Initialize( Memory.GetAllocator(), LogicalDevice.GetHandle(), InVertices, InIndices );
    return MeshRegistry.Insert( std::move( NewMesh ) );
}

LumenEngine::RHI::FPipelineHandle LumenEngine::VulkanRHI::FVulkanRHI::CreatePipeline ( const LumenEngine::FString &InVertexPath,
                                                                                       const LumenEngine::FString &InFragmentPath )
{
    FVulkanPipeline NewPipeline;
    const VkFormat &SwapChainFormat                = SwapChain.GetImageFormat();
    const VkDescriptorSetLayout &GlobalSetLayout   = Memory.GetGlobalSetLayout();
    const FPipelineDescription PipelineDescription = FVulkanPipeline::CreateDefaultDescription( InVertexPath, InFragmentPath, SwapChainFormat, GlobalSetLayout );

    if ( not NewPipeline.Initialize( LogicalDevice.GetHandle(), PipelineDescription ).has_value() )
    {
        return {};
    }

    return PipelineRegistry.Insert( std::move( NewPipeline ) );
}
