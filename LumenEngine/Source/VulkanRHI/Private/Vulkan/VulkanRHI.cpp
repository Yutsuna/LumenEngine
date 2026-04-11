/**
 * @file VulkanRHI.cpp
 * @brief Implementation of the Vulkan Render Hardware Interface.
 */

#include "Vulkan/VulkanRHI.hpp"
#include "Generic/GenericWindow.hpp"
#include "Vulkan/VulkanCore.hpp"
#include "Vulkan/VulkanDescriptorWriter.hpp"

#include <cstring>

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
    InitializeVMA();
    InitializeDescriptors();
    InitializeSwapChain( InWindow );
    InitializeCommandBuffers();

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

    for ( std::pair<const LumenEngine::UInt32, LumenEngine::VulkanRHI::FVulkanPipeline> &Pair : PipelineRegistry )
    {
        Pair.second.Cleanup( LogicalDevice.GetHandle() );
    }
    PipelineRegistry.clear();

    for ( std::pair<const LumenEngine::UInt32, LumenEngine::VulkanRHI::FVulkanMesh> &Pair : MeshRegistry )
    {
        Pair.second.Cleanup( Allocator );
    }
    MeshRegistry.clear();

    DestroyCommandBuffers();
    DestroySwapChain();
    DestroyDescriptors();
    DestroyVMA();
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

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyVMA () noexcept
{
    if ( Allocator != VK_NULL_HANDLE )
    {
        vmaDestroyAllocator( Allocator );
        Allocator = VK_NULL_HANDLE;
    }
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyDescriptors () noexcept
{
    if ( DescriptorPool != VK_NULL_HANDLE )
    {
        vkDestroyDescriptorPool( LogicalDevice.GetHandle(), DescriptorPool, nullptr );
        DescriptorPool = VK_NULL_HANDLE;
    }

    if ( GlobalSetLayout != VK_NULL_HANDLE )
    {
        vkDestroyDescriptorSetLayout( LogicalDevice.GetHandle(), GlobalSetLayout, nullptr );
        GlobalSetLayout = VK_NULL_HANDLE;
    }

    for ( UInt32 Index = 0; Index < MaxFramesInFlight; ++Index )
    {
        if ( GlobalUniformBuffers[Index].Buffer != VK_NULL_HANDLE )
        {
            vmaDestroyBuffer( Allocator, GlobalUniformBuffers[Index].Buffer, GlobalUniformBuffers[Index].Allocation );
            GlobalUniformBuffers[Index].Buffer = VK_NULL_HANDLE;
        }
    }
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroySwapChain () noexcept
{
    SwapChain.Cleanup( LogicalDevice.GetHandle() );
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyCommandBuffers () noexcept
{
    CommandPool.Cleanup( LogicalDevice.GetHandle() );
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

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeVMA ()
{
    VmaVulkanFunctions VulkanFunctions    = {};
    VulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    VulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo AllocatorInfo = {};
    AllocatorInfo.physicalDevice         = PhysicalDevice.GetHandle();
    AllocatorInfo.device                 = LogicalDevice.GetHandle();
    AllocatorInfo.instance               = Instance.GetHandle();
    AllocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
    AllocatorInfo.pVulkanFunctions       = &VulkanFunctions;
    AllocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    LUMEN_VK_CHECK( vmaCreateAllocator( &AllocatorInfo, &Allocator ) );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeDescriptors ()
{
    VkDescriptorSetLayoutBinding UboBinding{};
    UboBinding.binding         = 0;
    UboBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboBinding.descriptorCount = 1;
    UboBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo LayoutInfo{};
    LayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LayoutInfo.bindingCount = 1;
    LayoutInfo.pBindings    = &UboBinding;

    LUMEN_VK_CHECK( vkCreateDescriptorSetLayout( LogicalDevice.GetHandle(), &LayoutInfo, nullptr, &GlobalSetLayout ) );

    VkDescriptorPoolSize PoolSize{};
    PoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    PoolSize.descriptorCount = static_cast<UInt32>( MaxFramesInFlight );

    VkDescriptorPoolCreateInfo PoolInfo{};
    PoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.poolSizeCount = 1;
    PoolInfo.pPoolSizes    = &PoolSize;
    PoolInfo.maxSets       = static_cast<UInt32>( MaxFramesInFlight );

    LUMEN_VK_CHECK( vkCreateDescriptorPool( LogicalDevice.GetHandle(), &PoolInfo, nullptr, &DescriptorPool ) );

    VkBufferCreateInfo BufferInfo{};
    BufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size        = sizeof( RHI::FGlobalUniformData );
    BufferInfo.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo AllocInfo{};
    AllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    AllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    for ( UInt32 Index = 0; Index < MaxFramesInFlight; ++Index )
    {
        LUMEN_VK_CHECK( vmaCreateBuffer( Allocator, &BufferInfo, &AllocInfo, &GlobalUniformBuffers[Index].Buffer, &GlobalUniformBuffers[Index].Allocation,
                                         &GlobalUniformBuffers[Index].AllocationInfo ) );
    }

    TVector<VkDescriptorSetLayout> Layouts( MaxFramesInFlight, GlobalSetLayout );
    VkDescriptorSetAllocateInfo AllocSetInfo{};
    AllocSetInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    AllocSetInfo.descriptorPool     = DescriptorPool;
    AllocSetInfo.descriptorSetCount = static_cast<UInt32>( MaxFramesInFlight );
    AllocSetInfo.pSetLayouts        = Layouts.data();

    LUMEN_VK_CHECK( vkAllocateDescriptorSets( LogicalDevice.GetHandle(), &AllocSetInfo, GlobalDescriptorSets ) );

    for ( UInt32 Index = 0; Index < MaxFramesInFlight; ++Index )
    {
        FVulkanDescriptorWriter Writer;
        Writer.WriteBuffer( 0, GlobalUniformBuffers[Index].Buffer, sizeof( RHI::FGlobalUniformData ), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER );
        Writer.UpdateSet( LogicalDevice.GetHandle(), GlobalDescriptorSets[Index] );
    }
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
    return Allocator;
}

LumenEngine::VulkanRHI::FVulkanSwapChain &LumenEngine::VulkanRHI::FVulkanRHI::GetSwapChain () noexcept
{
    return SwapChain;
}

LumenEngine::RHI::IRHICommandList &LumenEngine::VulkanRHI::FVulkanRHI::GetCommandList () noexcept
{
    return CommandListImpl;
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeCommandBuffers ()
{
    CommandPool.Initialize( LogicalDevice.GetHandle(), LogicalDevice.GetGraphicsQueueFamily() );

    for ( LumenEngine::UInt32 Index = 0; Index < MaxFramesInFlight; ++Index )
    {
        CommandBuffers[Index] = CommandPool.AllocateBuffer( LogicalDevice.GetHandle() );
    }
}

void LumenEngine::VulkanRHI::FVulkanRHI::WaitIdle () const noexcept
{
    LogicalDevice.WaitIdle();
}

LumenEngine::Bool LumenEngine::VulkanRHI::FVulkanRHI::BeginFrame ( const RHI::FGlobalUniformData &InUniforms )
{
    const LumenEngine::UInt32 CurrentFrame = FrameIndex % MaxFramesInFlight;
    VkDevice Device                        = LogicalDevice.GetHandle();

    SwapChain.BeginFrame( Device, CurrentFrame );

    std::pair<VkImage, LumenEngine::UInt32> NextImagePair = SwapChain.AcquireNextImage( Device, CurrentFrame );
    if ( NextImagePair.first == VK_NULL_HANDLE )
    {
        return false;
    }

    CurrentImageIndex = NextImagePair.second;
    SwapChain.ResetFences( Device, CurrentFrame );

    // Synchronize global uniform data mapping for the current frame in flight
    std::memcpy( GlobalUniformBuffers[CurrentFrame].AllocationInfo.pMappedData, &InUniforms, sizeof( RHI::FGlobalUniformData ) );

    LUMEN_VK_CHECK( vkResetCommandBuffer( CommandBuffers[CurrentFrame].GetHandle(), 0 ) );

    CommandBuffers[CurrentFrame].Begin( VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT );

    /** Inform the CommandList which command buffer to use for this frame */
    CommandListImpl.SetActiveCommandBuffer( CommandBuffers[CurrentFrame].GetHandle() );

    return true;
}

void LumenEngine::VulkanRHI::FVulkanRHI::BeginRenderingInternal ( VkCommandBuffer InCmd, const LumenEngine::Float32 InClearColor[4] ) noexcept
{
    const LumenEngine::UInt32 CurrentFrame = FrameIndex % MaxFramesInFlight;
    const VkImage &Image                   = SwapChain.GetImages()[CurrentImageIndex];

    /** Transition swapchain image to color attachment layout for dynamic rendering */
    CommandBuffers[CurrentFrame].TransitionImageLayout( Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT );

    VkRenderingAttachmentInfo ColorAttachment{};
    ColorAttachment.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    ColorAttachment.imageView        = SwapChain.GetImageView( CurrentImageIndex );
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
    VkViewport Viewport{
        0.F, 0.F, static_cast<LumenEngine::Float32>( RenderInfo.renderArea.extent.width ), static_cast<LumenEngine::Float32>( RenderInfo.renderArea.extent.height ),
        0.F, 1.F };
    VkRect2D Scissor{ { 0, 0 }, RenderInfo.renderArea.extent };

    vkCmdSetViewport( InCmd, 0, 1, &Viewport );
    vkCmdSetScissor( InCmd, 0, 1, &Scissor );
}

void LumenEngine::VulkanRHI::FVulkanRHI::BindPipelineInternal ( VkCommandBuffer InCmd, const LumenEngine::RHI::FPipelineHandle InPipeline ) noexcept
{
    if ( not InPipeline.IsValid() || not PipelineRegistry.contains( InPipeline.ID ) )
    {
        return;
    }

    PipelineRegistry[InPipeline.ID].Bind( InCmd );

    // Bind Global Descriptor Set associated with the current frame
    const UInt32 CurrentFrame = FrameIndex % MaxFramesInFlight;
    vkCmdBindDescriptorSets( InCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineRegistry[InPipeline.ID].GetLayout(), 0, 1, &GlobalDescriptorSets[CurrentFrame], 0, nullptr );
}

void LumenEngine::VulkanRHI::FVulkanRHI::DrawMeshInternal ( VkCommandBuffer InCmd, const LumenEngine::RHI::FMeshHandle InMesh ) noexcept
{
    if ( not InMesh.IsValid() || not MeshRegistry.contains( InMesh.ID ) )
    {
        return;
    }

    MeshRegistry[InMesh.ID].BindAndDraw( InCmd );
}

void LumenEngine::VulkanRHI::FVulkanRHI::EndFrame ()
{
    const LumenEngine::UInt32 CurrentFrame = FrameIndex % MaxFramesInFlight;
    const VkImage &Image                   = SwapChain.GetImages()[CurrentImageIndex];

    CommandBuffers[CurrentFrame].TransitionImageLayout( Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT );
    CommandBuffers[CurrentFrame].End();

    SwapChain.SubmitAndPresent( CommandBuffers[CurrentFrame].GetHandle(), LogicalDevice.GetGraphicsQueue(), CurrentFrame, CurrentImageIndex );
    ++FrameIndex;
}

LumenEngine::RHI::FMeshHandle LumenEngine::VulkanRHI::FVulkanRHI::CreateMesh ( const LumenEngine::TVector<LumenEngine::Maths::FVertex> &InVertices,
                                                                               const LumenEngine::TVector<LumenEngine::UInt32> &InIndices )
{
    LumenEngine::VulkanRHI::FVulkanMesh NewMesh;
    NewMesh.Initialize( Allocator, InVertices, InIndices );

    const LumenEngine::UInt32 Handle = NextMeshID++;
    MeshRegistry[Handle]             = NewMesh;

    return LumenEngine::RHI::FMeshHandle( Handle );
}

LumenEngine::RHI::FPipelineHandle LumenEngine::VulkanRHI::FVulkanRHI::CreatePipeline ( const LumenEngine::FString &InVertexPath,
                                                                                       const LumenEngine::FString &InFragmentPath )
{
    LumenEngine::VulkanRHI::FVulkanPipeline NewPipeline;
    const VkFormat ColorFormat = SwapChain.GetImageFormat();

    if ( not NewPipeline.Initialize( LogicalDevice.GetHandle(), ColorFormat, InVertexPath, InFragmentPath, GlobalSetLayout ) )
    {
        return LumenEngine::RHI::FPipelineHandle();
    }

    const LumenEngine::UInt32 Handle = NextPipelineID++;
    PipelineRegistry[Handle]         = NewPipeline;

    return LumenEngine::RHI::FPipelineHandle( Handle );
}