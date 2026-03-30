/**
 * @file VulkanRHI.cpp
 * @brief Implementation of the FVulkanRHI class for Vulkan rendering context management.
 */

#include "VulkanRHI.hpp"
#include "VulkanDevice.hpp"
#include "VulkanResource.hpp"
#include "LaunchEngineLoop.hpp"
#include "Logging/Logger.hpp"

static LumenEngine::RHI::FVulkanRHI *GRHIInstance = nullptr;

LumenEngine::RHI::FVulkanRHI::FVulkanRHI () noexcept
{
    GRHIInstance = this;
    Device       = MakeUnique<FVulkanDevice>();
}

LumenEngine::RHI::FVulkanRHI::~FVulkanRHI () noexcept
{
    Shutdown();
    GRHIInstance = nullptr;
}

LumenEngine::RHI::FVulkanRHI &LumenEngine::RHI::FVulkanRHI::Get ()
{
    return *GRHIInstance;
}

LumenEngine::TExpected<void, LumenEngine::FString> LumenEngine::RHI::FVulkanRHI::Initialize ( void *InWindowHandle )
{
    LUMEN_LOG_INFO( LogVulkanRHI, "Initializing Vulkan RHI..." );

    auto Result = CreateInstance();
    if ( !Result ) return Result;

    Result = CreateSurface( InWindowHandle );
    if ( !Result ) return Result;

    return Device->Initialize( Instance, Surface );
}

void LumenEngine::RHI::FVulkanRHI::PostInitialize () noexcept
{
    LUMEN_LOG_INFO( LogVulkanRHI, "Vulkan RHI Post-Initialization..." );
}

void LumenEngine::RHI::FVulkanRHI::Shutdown () noexcept
{
    LUMEN_LOG_INFO( LogVulkanRHI, "Shutting down Vulkan RHI..." );

    // Force cleanup of all pending resources
    for ( auto *Resource : PendingDeletionQueue )
    {
        Resource->Release();
    }
    PendingDeletionQueue.Clear();

    Device.Reset();

    if ( Surface != VK_NULL_HANDLE )
    {
        vkDestroySurfaceKHR( Instance, Surface, nullptr );
        Surface = VK_NULL_HANDLE;
    }

    if ( Instance != VK_NULL_HANDLE )
    {
        vkDestroyInstance( Instance, nullptr );
        Instance = VK_NULL_HANDLE;
    }
}

void LumenEngine::RHI::FVulkanRHI::RHIBeginFrame ()
{
    TickDeferredDeletion();
}

void LumenEngine::RHI::FVulkanRHI::RHIEndFrame ()
{
    // Presentation logic would go here, calling the SwapChain
}

LumenEngine::RHI::FVulkanDevice &LumenEngine::RHI::FVulkanRHI::GetDevice () noexcept
{
    return *Device;
}

void LumenEngine::RHI::FVulkanRHI::DeferredDeletion ( FVulkanResource *Resource )
{
    PendingDeletionQueue.PushBack( Resource );
}

void LumenEngine::RHI::FVulkanRHI::TickDeferredDeletion ()
{
    const UInt64 CurrentFrame = GEngineLoop.GetFrameIndex();

    for ( ISize Index = 0; Index < static_cast<ISize>( PendingDeletionQueue.GetSize() ); )
    {
        FVulkanResource *Resource = PendingDeletionQueue[Index];

        // If the resource has been pending for at least MAX_FRAMES_IN_FLIGHT, it's safe to delete
        if ( CurrentFrame > Resource->DeletionFrame + MAX_FRAMES_IN_FLIGHT )
        {
            Resource->Release();
            PendingDeletionQueue.Erase( PendingDeletionQueue.Begin() + Index );
            // Don't increment Index, as the next element shifted into this position
        }
        else
        {
            ++Index;
        }
    }
}

LumenEngine::TExpected<void, LumenEngine::FString> LumenEngine::RHI::FVulkanRHI::CreateInstance ()
{
    // Implementation for instance creation (minimal for now)
    VkApplicationInfo AppInfo{};
    AppInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName   = "Lumen Engine";
    AppInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    AppInfo.pEngineName        = "Lumen Engine";
    AppInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    AppInfo.apiVersion         = VK_API_VERSION_1_3;

    VkInstanceCreateInfo CreateInfo{};
    CreateInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CreateInfo.pApplicationInfo = &AppInfo;

    // Add necessary extensions (e.g., for surface support)
    // This would normally be platform-specific
    TVector<const AnsiChar *> Extensions = { VK_KHR_SURFACE_EXTENSION_NAME };
#if defined( LUMEN_ENGINE_PLATFORM_LINUX )
    Extensions.PushBack( "VK_KHR_XCB_SURFACE" ); // Example
#endif

    CreateInfo.enabledExtensionCount   = static_cast<UInt32>( Extensions.GetSize() );
    CreateInfo.ppEnabledExtensionNames = Extensions.GetData();

    VULKAN_CHECK( vkCreateInstance( &CreateInfo, nullptr, &Instance ) );

    return {};
}

LumenEngine::TExpected<void, LumenEngine::FString> LumenEngine::RHI::FVulkanRHI::CreateSurface ( void *InWindowHandle )
{
    // Platform-specific surface creation would go here
    // For now, we'll assume it's handled by ApplicationCore or passed in
    return {};
}
