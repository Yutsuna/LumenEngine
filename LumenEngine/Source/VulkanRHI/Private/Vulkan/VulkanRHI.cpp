/**
 * @file VulkanRHI.cpp
 * @brief Implementation of the Vulkan Render Hardware Interface.
 */

#include "Vulkan/VulkanRHI.hpp"

#include "Container/String.hpp"

#include "Generic/GenericWindow.hpp"

#include "Vulkan/VulkanCore.hpp"
#include "Vulkan/VulkanSceneRenderer.hpp"

#ifndef LUMEN_GPU_CULL_SHADER_PATH
    #define LUMEN_GPU_CULL_SHADER_PATH ""
#endif

LumenEngine::VulkanRHI::FVulkanRHI::FVulkanRHI () noexcept : CommandListImpl( this )
{
    /* Empty */
}

/**
 * Public Methods
 */

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
    InitializeGpuDrivenResources();

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

    DeferredDestructionQueue.Shutdown();

    PipelineRegistry.ForEach( [this] ( FVulkanPipeline &Pipeline ) { Pipeline.Cleanup( LogicalDevice.GetHandle() ); } );
    PipelineRegistry.Clear();

    MeshRegistry.ForEach( [this] ( FVulkanMesh &Mesh ) { Mesh.Cleanup( Memory.GetAllocator() ); } );
    MeshRegistry.Clear();

    ShutdownGpuDrivenResources();

    FrameContext.Shutdown( LogicalDevice.GetHandle() );

    DestroySwapChain();

    Memory.Shutdown( LogicalDevice.GetHandle() );

    DestroyVulkanDevice();
    DestroyVulkanInstance();

    bIsInitialized = false;
    LUMEN_LOG_INFO( LogVulkanRHI, "Vulkan RHI shut down." );
}

/**
 * Private Methods
 */

/**
 * Vulkan Cleanup Methods
 */

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

/**
 * Vulkan Initialization Methods
 */

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

    const LumenEngine::UInt64 AbsoluteFrame = FrameContext.GetAbsoluteFrameIndex();
    DeferredDestructionQueue.Tick( AbsoluteFrame );

    const LumenEngine::UInt32 CurrentFrame = FrameContext.GetCurrentFrameIndex();

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
        .width    = static_cast<Float32>( RenderInfo.renderArea.extent.width ),
        .height   = static_cast<Float32>( RenderInfo.renderArea.extent.height ),
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

void LumenEngine::VulkanRHI::FVulkanRHI::DrawSceneInternal ( VkCommandBuffer InCmd,
                                                             const LumenEngine::RHI::FSceneSnapshot &InSceneSnapshot,
                                                             const LumenEngine::Float32 InClearColor[4] ) noexcept
{

    /** INFO: Delegate actual rendering to the specialized SceneRenderer sub-system */
    const UInt32 CurrentFrame = FrameContext.GetCurrentFrameIndex();

    VulkanSceneRenderer::PrepareScene( InCmd, InSceneSnapshot, CurrentFrame, MeshRegistry, PipelineRegistry, Memory, SceneBuffer, IndirectBuffer, CullingPass );

    /** INFO: Start a dynamic rendering region */
    BeginRenderingInternal( InCmd, InClearColor );

    VulkanSceneRenderer::RenderScene( InCmd, InSceneSnapshot, CurrentFrame, MeshRegistry, PipelineRegistry, Memory, IndirectBuffer, CullingPass );

    /** INFO: End dynamic rendering */
    vkCmdEndRendering( InCmd );
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

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyMesh ( RHI::FMeshHandle InHandle )
{
    if ( not MeshRegistry.IsValid( InHandle ) )
    {
        return;
    }

    FVulkanMesh *Mesh                       = MeshRegistry.Get( InHandle );
    const LumenEngine::UInt64 AbsoluteFrame = FrameContext.GetAbsoluteFrameIndex();

    /** Capture the resource data to cleanup later */
    FVulkanMesh MeshToDestroy = *Mesh;
    MeshRegistry.Remove( InHandle );

    DeferredDestructionQueue.Enqueue( [this, MeshToCleanup = MeshToDestroy] () mutable { MeshToCleanup.Cleanup( Memory.GetAllocator() ); }, AbsoluteFrame );
}

LumenEngine::RHI::FPipelineHandle LumenEngine::VulkanRHI::FVulkanRHI::CreatePipeline ( const LumenEngine::FString &InVertexPath,
                                                                                       const LumenEngine::FString &InFragmentPath )
{
    Compiler::FShaderCompileResult VResult = RuntimeCompiler->CompileShader( Compiler::FShaderCompileRequestBuilder().Path( InVertexPath ).Vertex().Build() );
    if ( not VResult.IsSuccess() )
    {
        LUMEN_LOG_ERROR( LogVulkanRHI, "Failed to compile vertex shader (Path: {}, Log: {}).", InVertexPath, VResult.ErrorLog.c_str() );
        return {};
    }

    Compiler::FShaderCompileResult FResult = RuntimeCompiler->CompileShader( Compiler::FShaderCompileRequestBuilder().Path( InFragmentPath ).Fragment().Build() );
    if ( not FResult.IsSuccess() )
    {
        LUMEN_LOG_ERROR( LogVulkanRHI, "Failed to compile fragment shader (Path: {}, Log: {}).", InFragmentPath, FResult.ErrorLog.c_str() );
        return {};
    }

    FVulkanPipeline NewPipeline;
    VkFormat ImageFormat                   = SwapChain.GetImageFormat();
    VkDescriptorSetLayout GlobalSetLayout  = Memory.GetGlobalSetLayout();
    const FPipelineDescription Description = FVulkanPipeline::CreateDefaultDescription( InVertexPath, InFragmentPath, ImageFormat, GlobalSetLayout );

    if ( not NewPipeline.Initialize( LogicalDevice.GetHandle(), Description, VResult.Shader->SpirV, FResult.Shader->SpirV ).has_value() )
    {
        return {};
    }

    return PipelineRegistry.Insert( NewPipeline );
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyPipeline ( RHI::FPipelineHandle InHandle )
{
    if ( not PipelineRegistry.IsValid( InHandle ) )
    {
        return;
    }

    FVulkanPipeline *Pipeline               = PipelineRegistry.Get( InHandle );
    const LumenEngine::UInt64 AbsoluteFrame = FrameContext.GetAbsoluteFrameIndex();

    /** Capture the resource data to cleanup later */
    FVulkanPipeline PipelineToDestroy = *Pipeline;
    PipelineRegistry.Remove( InHandle );

    DeferredDestructionQueue.Enqueue( [this, PipelineToCleanup = PipelineToDestroy] () mutable { PipelineToCleanup.Cleanup( LogicalDevice.GetHandle() ); },
                                      AbsoluteFrame );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeGpuDrivenResources ()
{
    SceneBuffer.Initialize( Memory.GetAllocator(), LogicalDevice.GetHandle(), Memory.GetDescriptorPool(), Memory.GetSceneSetLayout() );
    IndirectBuffer.Initialize( Memory.GetAllocator(), LogicalDevice.GetHandle(), Memory.GetDescriptorPool(), Memory.GetCullSetLayout() );

    Compiler::FShaderCompilerConfig CompilerConfig;
    RuntimeCompiler = MakeUnique<Compiler::FShaderCompiler>( std::move( CompilerConfig ) );

    Compiler::FShaderCompileRequestBuilder RequestBuilder;
    RequestBuilder.Path( LUMEN_GPU_CULL_SHADER_PATH ).Compute().Macro( "MAX_INSTANCES", std::format( "{}U", FGPUSceneBuffer::MaxInstances ) );

    const Compiler::FShaderCompileResult CompileResult = RuntimeCompiler->CompileShader( RequestBuilder.Build() );

    if ( CompileResult.IsSuccess() )
    {
        const Compiler::FCompiledShader &CompiledShader = *CompileResult.Shader;

        LUMEN_LOG_INFO( LogVulkanRHI, "GPU Culling shader JIT successful (Source: {}, Hash: {:016x}).", CompiledShader.bFromCache ? "Disk Cache" : "Freshly Compiled",
                        CompiledShader.Hash );

        const Bool bCullingPassReady =
            CullingPass.Initialize( LogicalDevice.GetHandle(), Memory.GetGlobalSetLayout(), Memory.GetSceneSetLayout(), Memory.GetCullSetLayout(), CompiledShader.SpirV );

        if ( bCullingPassReady )
        {
            LUMEN_LOG_INFO( LogVulkanRHI, "GPU-driven culling sub-system fully initialized." );
        }
        else
        {
            LUMEN_LOG_ERROR( LogVulkanRHI, "GPUCullingPass: Failed to create compute pipeline from SPIR-V bytecode." );
        }
    }
    else
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Critical Failure: GPU Culling shader compilation failed!\nLog Output:\n{}", CompileResult.ErrorLog.c_str() );
    }
}

void LumenEngine::VulkanRHI::FVulkanRHI::ShutdownGpuDrivenResources () noexcept
{
    CullingPass.Shutdown( LogicalDevice.GetHandle() );
    IndirectBuffer.Shutdown( Memory.GetAllocator() );
    SceneBuffer.Shutdown( Memory.GetAllocator(), LogicalDevice.GetHandle() );
}
