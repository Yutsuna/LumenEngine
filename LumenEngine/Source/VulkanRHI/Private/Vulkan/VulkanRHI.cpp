/**
 * @file VulkanRHI.cpp
 * @brief Implementation of the Vulkan Render Hardware Interface.
 */

#include "Vulkan/VulkanRHI.hpp"

#include "Container/String.hpp"

#include "Generic/GenericWindow.hpp"

#include "Vulkan/VulkanCore.hpp"
#include "Vulkan/VulkanRenderContextManager.hpp"
#include "Vulkan/VulkanSceneRenderer.hpp"

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
    const FDescriptorConfig Config{ .MaxFramesInFlight = MaxFramesInFlight };
    Memory.Initialize( Instance.GetHandle(), PhysicalDevice.GetHandle(), LogicalDevice.GetHandle(), Config );
    InitializeSwapChain( InWindow );
    FrameContext.Initialize( LogicalDevice.GetHandle(), LogicalDevice.GetGraphicsQueueFamily() );

    /** Initialize MSAA Target and default parameters */
    MsaaManager.Initialize( PhysicalDevice.GetHandle(), Memory.GetAllocator(), LogicalDevice.GetHandle(), SwapChain.GetImageFormat(), SwapChain.GetExtent(),
                            CurrentSettings );

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

    TextureRegistry.ForEach( [this] ( FVulkanImage &Image ) { Image.Cleanup( Memory.GetAllocator(), LogicalDevice.GetHandle() ); } );
    TextureRegistry.Clear();

    ShutdownGpuDrivenResources();

    MsaaManager.Shutdown( Memory.GetAllocator(), LogicalDevice.GetHandle() );

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

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeVulkanInstance ( const TSharedPtr<LumenEngine::FGenericWindow> &InWindow )
{
    Instance.Initialize( InWindow );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeVulkanDevice ()
{
    const LumenEngine::TVector<const LumenEngine::AnsiChar *> DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    PhysicalDevice.Initialize( Instance.GetHandle(), Instance.GetSurface() );
    LogicalDevice.Initialize( PhysicalDevice.GetHandle(), Instance.GetSurface(), DeviceExtensions );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeSwapChain ( const TSharedPtr<LumenEngine::FGenericWindow> &InWindow )
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

    const LumenEngine::UInt32 NumFrames = Memory.GetNumFramesInFlight();
    if ( NumFrames == 0U )
    {
        return false;
    }

    const LumenEngine::UInt32 CurrentFrame = FrameContext.GetCurrentFrameIndex();
    if ( CurrentFrame >= NumFrames )
    {
        return false;
    }

    Memory.UpdateGlobalUniformData( CurrentFrame, InUniforms );

    CommandListImpl.SetActiveCommandBuffer( FrameContext.GetCurrentCommandBuffer().GetHandle() );

    return true;
}

void LumenEngine::VulkanRHI::FVulkanRHI::BeginRenderingInternal ( VkCommandBuffer InCmd, const LumenEngine::Float32 InClearColor[4] ) noexcept
{
    const VkImage &Image    = SwapChain.GetImages()[FrameContext.GetCurrentImageIndex()];
    const VkExtent2D Extent = SwapChain.GetExtent();

    MsaaManager.RecreateRenderTargetIfNeeded( Memory.GetAllocator(), LogicalDevice.GetHandle(), SwapChain.GetImageFormat(), Extent );

    VulkanRenderContextManager::BeginRendering( InCmd, Image, SwapChain.GetImageView( FrameContext.GetCurrentImageIndex() ), SwapChain.GetImageFormat(), Extent,
                                                MsaaManager, InClearColor );
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

    if ( GlobalDescriptorSet == VK_NULL_HANDLE )
    {
        return;
    }

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
    const UInt32 CurrentFrame = FrameContext.GetCurrentFrameIndex();

    VulkanSceneRenderer::PrepareScene( InCmd, InSceneSnapshot, CurrentFrame, MeshRegistry, PipelineRegistry, Memory, SceneBuffer, IndirectBuffer, CullingPass );

    BeginRenderingInternal( InCmd, InClearColor );

    VulkanSceneRenderer::RenderScene( InCmd, InSceneSnapshot, CurrentFrame, MeshRegistry, PipelineRegistry, Memory, IndirectBuffer, CullingPass );

    vkCmdEndRendering( InCmd );
}

void LumenEngine::VulkanRHI::FVulkanRHI::EndFrame ()
{
    const VkImage &Image = SwapChain.GetImages()[FrameContext.GetCurrentImageIndex()];
    VkCommandBuffer Cmd  = FrameContext.GetCurrentCommandBuffer().GetHandle();

    VulkanRenderContextManager::TransitionPresentImageToPresentSource( Cmd, Image );

    FrameContext.SubmitAndPresent( SwapChain, LogicalDevice.GetGraphicsQueue() );
}

LumenEngine::RHI::FMeshHandle LumenEngine::VulkanRHI::FVulkanRHI::CreateMesh ( const LumenEngine::TVector<LumenEngine::Maths::FVertex> &InVertices,
                                                                               const LumenEngine::TVector<LumenEngine::UInt32> &InIndices )
{
    LumenEngine::VulkanRHI::FVulkanMesh NewMesh;

    NewMesh.Initialize( Memory.GetAllocator(), LogicalDevice.GetHandle(), InVertices, InIndices );
    return MeshRegistry.Insert( NewMesh );
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyMesh ( RHI::FMeshHandle InHandle )
{
    if ( not MeshRegistry.IsValid( InHandle ) )
    {
        return;
    }

    FVulkanMesh *Mesh                       = MeshRegistry.Get( InHandle );
    const LumenEngine::UInt64 AbsoluteFrame = FrameContext.GetAbsoluteFrameIndex();

    FVulkanMesh MeshToDestroy = *Mesh;
    MeshRegistry.Remove( InHandle );

    DeferredDestructionQueue.Enqueue( [this, MeshToCleanup = MeshToDestroy] () mutable { MeshToCleanup.Cleanup( Memory.GetAllocator() ); }, AbsoluteFrame );
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyTexture ( RHI::FTextureHandle InHandle )
{
    if ( not TextureRegistry.IsValid( InHandle ) )
    {
        return;
    }

    FVulkanImage *Image                     = TextureRegistry.Get( InHandle );
    const LumenEngine::UInt64 AbsoluteFrame = FrameContext.GetAbsoluteFrameIndex();

    FVulkanImage ImageToDestroy = *Image;
    TextureRegistry.Remove( InHandle );

    DeferredDestructionQueue.Enqueue( [this, ImageToCleanup = ImageToDestroy] () mutable { ImageToCleanup.Cleanup( Memory.GetAllocator(), LogicalDevice.GetHandle() ); },
                                      AbsoluteFrame );
}

LumenEngine::RHI::FPipelineHandle LumenEngine::VulkanRHI::FVulkanRHI::CreatePipeline ( const RHI::FGraphicsPipelineDesc &InDescription )
{
    FVulkanPipeline NewPipeline;
    VkFormat ImageFormat                   = SwapChain.GetImageFormat();
    VkDescriptorSetLayout GlobalSetLayout  = Memory.GetGlobalSetLayout();
    const FPipelineDescription Description = FVulkanPipeline::CreateDefaultDescription( ImageFormat, GlobalSetLayout, MsaaManager.GetActiveSamples() );

    if ( not NewPipeline.Initialize( LogicalDevice.GetHandle(), Description, InDescription.VertexShader, InDescription.FragmentShader ).has_value() )
    {
        LUMEN_LOG_ERROR( LogVulkanRHI, "Failed to create graphics pipeline from provided SPIR-V bytecode." );
        return {};
    }

    return PipelineRegistry.Insert( std::move( NewPipeline ) );
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyPipeline ( RHI::FPipelineHandle InHandle )
{
    if ( not PipelineRegistry.IsValid( InHandle ) )
    {
        return;
    }

    FVulkanPipeline *Pipeline               = PipelineRegistry.Get( InHandle );
    const LumenEngine::UInt64 AbsoluteFrame = FrameContext.GetAbsoluteFrameIndex();

    FVulkanPipeline PipelineToDestroy = *Pipeline;
    PipelineRegistry.Remove( InHandle );

    DeferredDestructionQueue.Enqueue( [this, PipelineToCleanup = PipelineToDestroy] () mutable { PipelineToCleanup.Cleanup( LogicalDevice.GetHandle() ); },
                                      AbsoluteFrame );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeGpuDrivenResources ( const RHI::FShaderByteCode &InComputeCode )
{
    if ( Memory.GetDescriptorPool() == VK_NULL_HANDLE )
    {
        return;
    }

    SceneBuffer.Initialize( Memory.GetAllocator(), LogicalDevice.GetHandle(), Memory.GetDescriptorPool(), Memory.GetSceneSetLayout() );
    IndirectBuffer.Initialize( Memory.GetAllocator(), LogicalDevice.GetHandle(), Memory.GetDescriptorPool(), Memory.GetCullSetLayout() );

    const Bool bCullingPassReady =
        CullingPass.Initialize( LogicalDevice.GetHandle(), Memory.GetGlobalSetLayout(), Memory.GetSceneSetLayout(), Memory.GetCullSetLayout(), InComputeCode );

    if ( bCullingPassReady )
    {
        LUMEN_LOG_INFO( LogVulkanRHI, "GPU-driven culling sub-system fully initialized." );
    }
    else
    {
        LUMEN_LOG_ERROR( LogVulkanRHI, "GPUCullingPass: Failed to create compute pipeline from SPIR-V bytecode." );
    }
}

void LumenEngine::VulkanRHI::FVulkanRHI::ShutdownGpuDrivenResources () noexcept
{
    CullingPass.Shutdown( LogicalDevice.GetHandle() );
    IndirectBuffer.Shutdown( Memory.GetAllocator() );
    SceneBuffer.Shutdown( Memory.GetAllocator(), LogicalDevice.GetHandle() );
}

void LumenEngine::VulkanRHI::FVulkanRHI::SetVisualSettings ( const RHI::FVisualSettings &InSettings )
{
    const UInt64 AbsoluteFrame = FrameContext.GetAbsoluteFrameIndex();

    MsaaManager.SetVisualSettings( InSettings, PhysicalDevice.GetHandle(), LogicalDevice.GetHandle(), Memory.GetAllocator(), SwapChain.GetImageFormat(),
                                   SwapChain.GetExtent(), DeferredDestructionQueue, PipelineRegistry, AbsoluteFrame );

    CurrentSettings = MsaaManager.GetCurrentSettings();
}
