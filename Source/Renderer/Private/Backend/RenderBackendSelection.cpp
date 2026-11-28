/**
 * @file RenderBackendSelection.cpp
 * @brief Moves the former Lumina composition-root backend setup behind the renderer boundary.
 */

#include <Backend/RenderBackendSelection.hpp>

#include <Logging/Logger.hpp>
#include <Templates/Unexpected.hpp>

#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Module/RHIAdapterInfo.hpp>
#include <RHI/Types/RHICapabilities.hpp>

#include <RHIOpenGL/RHIOpenGLModule.hpp>
#include <RHIOpenGL/RHIOpenGLSurfaceBridge.hpp>

namespace
{

LUMEN_LOG_DEFINE_CATEGORY( LogRenderBackend, "RenderBackend" );

/** @brief Maps the opaque surface identifier to the backend-local window-system tag. */
[[nodiscard]] Lumen::TOptional<Lumen::RHIOpenGL::EOpenGLNativeWindowSystem>
ToOpenGLWindowSystem ( const Lumen::FStringView InSource ) noexcept
{
    if ( InSource == Lumen::FStringView( "Win32" ) )
    {
        return Lumen::TOptional<Lumen::RHIOpenGL::EOpenGLNativeWindowSystem>(
            Lumen::RHIOpenGL::EOpenGLNativeWindowSystem::Win32 );
    }
    if ( InSource == Lumen::FStringView( "X11" ) )
    {
        return Lumen::TOptional<Lumen::RHIOpenGL::EOpenGLNativeWindowSystem>( Lumen::RHIOpenGL::EOpenGLNativeWindowSystem::X11 );
    }
    if ( InSource == Lumen::FStringView( "Wayland" ) )
    {
        return Lumen::TOptional<Lumen::RHIOpenGL::EOpenGLNativeWindowSystem>(
            Lumen::RHIOpenGL::EOpenGLNativeWindowSystem::Wayland );
    }
    return {};
}

} // namespace

Lumen::TExpected<Lumen::FSelectedRenderBackend, Lumen::ERendererError>
Lumen::SelectRenderBackend ( const FRendererDesc &InDesc,
                             const RHI::FRHIExtent3D &InExtent,
                             TOptional<TSharedRef<RHI::IRHIDevice>> InInjectedDevice ) noexcept
{
    using namespace Lumen;

    FSelectedRenderBackend Selected;

    /** INFO: The internal test hook supplies a device directly; no module is created in that path. */
    if ( InInjectedDevice )
    {
        Selected.Device.Emplace( *InInjectedDevice );
        Selected.AdapterName = ( *InInjectedDevice )->GetDebugName();
        return Selected;
    }

    if ( InDesc.Backend == FStringView( "null" ) )
    {
        return MakeUnexpected( ERendererError::CapabilityUnsupported );
    }

    TOptional<RHIOpenGL::FOpenGLNativeWindowDesc> NativeWindow;
    if ( InDesc.Surface.bPresent )
    {
        const auto WindowSystem = ToOpenGLWindowSystem( InDesc.Surface.Source );
        if ( not WindowSystem or InDesc.Surface.NativeWindow == 0U )
        {
            return MakeUnexpected( ERendererError::InvalidDesc );
        }
        RHIOpenGL::FOpenGLNativeWindowDesc Desc;
        Desc.WindowSystem           = *WindowSystem;
        Desc.NativeWindow           = InDesc.Surface.NativeWindow;
        Desc.NativeDisplay          = InDesc.Surface.NativeDisplay;
        Desc.NativeWindowAttachment = InDesc.Surface.NativeWindowAttachment;
        Desc.Extent                 = InExtent;
        NativeWindow.Emplace( Desc );
    }

    const auto ModuleResult =
        NativeWindow ? RHIOpenGL::CreateRHIOpenGLModule( *NativeWindow ) : RHIOpenGL::CreateRHIOpenGLModule();
    if ( not ModuleResult ) [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogRenderBackend, "Backend module creation failed: {}", ModuleResult.Error().Message );
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }
    Selected.Module = *ModuleResult;

    auto Adapters = Selected.Module->EnumerateAdapters();
    if ( not Adapters or Adapters->empty() ) [[unlikely]]
    {
        if ( not Adapters )
        {
            LUMEN_LOG_ERROR( LogRenderBackend, "Adapter enumeration failed: {}", Adapters.Error().Message );
        }
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }

    const RHI::FRHIAdapterInfo *Chosen = nullptr;
    for ( const RHI::FRHIAdapterInfo &Adapter : *Adapters )
    {
        if ( Chosen == nullptr or
             ( Chosen->Type == RHI::ERHIAdapterType::Software and Adapter.Type != RHI::ERHIAdapterType::Software ) )
        {
            Chosen = &Adapter;
        }
    }
    Selected.bSoftwareAdapter = Chosen->Type == RHI::ERHIAdapterType::Software;
    Selected.AdapterName      = Chosen->Name;

    RHI::FRHIDeviceDesc DeviceDesc;
    DeviceDesc.AdapterId.Emplace( Chosen->AdapterId );
    DeviceDesc.RequiredCapabilities.Features =
        RHI::ERHICapabilityFlags::DynamicRendering | RHI::ERHICapabilityFlags::TimelineSemaphores;
    if ( InDesc.Surface.bPresent )
    {
        DeviceDesc.RequiredCapabilities.Features |= RHI::ERHICapabilityFlags::Presentation;
    }
    auto DeviceResult = Selected.Module->CreateDevice( DeviceDesc );
    if ( not DeviceResult ) [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogRenderBackend, "Device creation failed: {}", DeviceResult.Error().Message );
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }
    Selected.Device.Emplace( *DeviceResult );

    if ( InDesc.Surface.bPresent )
    {
        auto BridgeResult = RHIOpenGL::CreateRHIOpenGLSurfaceBridge( *NativeWindow );
        if ( not BridgeResult ) [[unlikely]]
        {
            LUMEN_LOG_ERROR( LogRenderBackend, "Surface creation failed: {}", BridgeResult.Error().Message );
            return MakeUnexpected( ERendererError::RHICreationFailed );
        }

        RHI::FRHISwapchainDesc SwapchainDesc;
        SwapchainDesc.Surface = {
            .Type             = RHI::ERHISurfaceType::Window,
            .NativeWindow     = InDesc.Surface.NativeWindow,
            .NativeDisplay    = InDesc.Surface.NativeDisplay,
            .bRequiresDisplay = InDesc.Surface.bRequiresDisplay,
            .Bridge           = *BridgeResult,
            .InitialExtent    = InExtent,
        };
        SwapchainDesc.Format     = RHI::ERHIFormat::RGBA8Unorm;
        SwapchainDesc.Extent     = InExtent;
        SwapchainDesc.ImageCount = 3U;
        auto SwapchainResult     = ( *Selected.Device )->CreateSwapchain( SwapchainDesc );
        if ( not SwapchainResult ) [[unlikely]]
        {
            LUMEN_LOG_ERROR( LogRenderBackend, "Swapchain creation failed: {}", SwapchainResult.Error().Message );
            return MakeUnexpected( ERendererError::RHICreationFailed );
        }
        Selected.Swapchain = *SwapchainResult;
    }

    return Selected;
}
