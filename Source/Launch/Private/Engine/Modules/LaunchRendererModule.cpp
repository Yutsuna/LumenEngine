/**
 * @file LaunchRendererModule.cpp
 * @brief Owns the renderer and translates portable window events into neutral resize requests.
 */

#include <Engine/Modules/LaunchRendererModule.hpp>

#include <Generic/GenericApplicationMessageHandler.hpp>
#include <Generic/GenericNativeWindowHandles.hpp>
#include <Generic/GenericWindow.hpp>

#include <Logging/Logger.hpp>
#include <Templates/Unexpected.hpp>

#include <utility>

namespace
{

LUMEN_LOG_DEFINE_CATEGORY( LogLaunchRenderer, "LaunchRenderer" );

/** @brief Maps AppCore's window-system enum to an opaque identifier string understood by Renderer/Private. */
[[nodiscard]] Lumen::FStringView WindowSystemName ( const Lumen::ENativeWindowSystem InSystem ) noexcept
{
    using namespace Lumen;
    switch ( InSystem )
    {
    case ENativeWindowSystem::Win32:
        return FStringView( "Win32" );
    case ENativeWindowSystem::X11:
        return FStringView( "X11" );
    case ENativeWindowSystem::Wayland:
        return FStringView( "Wayland" );
    case ENativeWindowSystem::Unknown:
    default:
        return {};
    }
}

/** @brief Converts portable native handles into the neutral surface description consumed by Renderer. */
[[nodiscard]] Lumen::TOptional<Lumen::FRenderSurfaceDesc>
ToRenderSurface ( const Lumen::FNativeWindowHandles &InHandles ) noexcept
{
    const Lumen::FStringView Source = WindowSystemName( InHandles.WindowSystem );
    if ( Source.empty() or InHandles.NativeWindow == 0U )
    {
        return {};
    }
    Lumen::FRenderSurfaceDesc Desc;
    Desc.Source                 = Lumen::FString( Source );
    Desc.NativeWindow           = InHandles.NativeWindow;
    Desc.NativeDisplay          = InHandles.NativeDisplay;
    Desc.NativeWindowAttachment = InHandles.NativeWindowAttachment;
    Desc.bRequiresDisplay       = InHandles.NativeDisplay != 0U;
    Desc.bPresent               = true;
    Desc.InitialExtent          = { InHandles.Width, InHandles.Height, 1U };
    return Lumen::TOptional<Lumen::FRenderSurfaceDesc>( Desc );
}

/** @brief Resize handler owned by the module, installed for its whole lifetime. */
class FLaunchRendererResizeHandler final : public Lumen::FGenericApplicationMessageHandler
{
public:

    FLaunchRendererResizeHandler ( Lumen::TSharedPtr<Lumen::FGenericApplicationMessageHandler> InNextHandler,
                                   Lumen::Launch::FLaunchRendererModule &InModule ) noexcept
        : NextHandler( std::move( InNextHandler ) ), Module( &InModule )
    {
    }

    /**
     * INFO: Every message that is not consumed here must be forwarded to the previous handler.
     *       Otherwise the chain stops at this decorator and exit / input requests never reach
     *       FLaunchMessageHandler.
     */

    void OnRequestExit () noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnRequestExit();
        }
    }

    void OnQuit () noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnQuit();
        }
    }

    void OnOSPaint ( const Lumen::TSharedRef<Lumen::FGenericWindow> &InWindow ) noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnOSPaint( InWindow );
        }
    }

    void OnWindowMoved ( const Lumen::TSharedRef<Lumen::FGenericWindow> &InWindow ) noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnWindowMoved( InWindow );
        }
    }

    void OnWindowResized ( const Lumen::TSharedRef<Lumen::FGenericWindow> &InWindow ) noexcept override
    {
        const auto Size = InWindow->GetWindowSize();
        if ( Size.X > 0 and Size.Y > 0 )
        {
            const auto Resized = Module->GetRenderer().RequestResize(
                { static_cast<Lumen::UInt32>( Size.X ), static_cast<Lumen::UInt32>( Size.Y ), 1U } );
            if ( not Resized and Resized.Error() != Lumen::ERendererError::QueueFull )
            {
                LUMEN_LOG_WARNING( LogLaunchRenderer, "Resize request rejected with error {}.",
                                   static_cast<Lumen::UInt32>( Resized.Error() ) );
            }
        }
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnWindowResized( InWindow );
        }
    }

    void OnWindowActivationChanged ( const Lumen::TSharedRef<Lumen::FGenericWindow> &InWindow ) noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnWindowActivationChanged( InWindow );
        }
    }

    void OnMouseDown ( const Lumen::EMouseButton::Type InMouseButton ) noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnMouseDown( InMouseButton );
        }
    }

    void OnMouseUp ( const Lumen::EMouseButton::Type InMouseButton ) noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnMouseUp( InMouseButton );
        }
    }

    void OnMouseDoubleClick ( const Lumen::EMouseButton::Type InMouseButton ) noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnMouseDoubleClick( InMouseButton );
        }
    }

    void OnMouseMove ( const Lumen::Int32 InX, const Lumen::Int32 InY ) noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnMouseMove( InX, InY );
        }
    }

    void OnMouseWheel ( const Lumen::Int32 InDelta ) noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnMouseWheel( InDelta );
        }
    }

    void OnKeyDown ( const Lumen::EKeys::Type InKey, const Lumen::Bool bIsRepeated ) noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnKeyDown( InKey, bIsRepeated );
        }
    }

    void OnKeyUp ( const Lumen::EKeys::Type InKey, const Lumen::Bool bIsRepeated ) noexcept override
    {
        if ( NextHandler.IsValid() )
        {
            NextHandler->OnKeyUp( InKey, bIsRepeated );
        }
    }

private:

    Lumen::TSharedPtr<Lumen::FGenericApplicationMessageHandler> NextHandler;
    Lumen::Launch::FLaunchRendererModule *Module = nullptr;
};

} // namespace

LUMEN_REGISTER_MODULE( Lumen::Launch::FLaunchRendererModule )

Lumen::Bool Lumen::Launch::FLaunchRendererModule::IsEnabled ( const FLaunchEngineConfig & ) noexcept
{
    return true;
}

Lumen::Launch::TLaunchResult<Lumen::TUniquePtr<Lumen::Launch::FLaunchRendererModule>>
Lumen::Launch::FLaunchRendererModule::Create ( const TModuleContext<FLaunchRendererModule> &InContext ) noexcept
{
    FRenderSurfaceDesc Surface;
    FLaunchWindowModule *const WindowModule = InContext.Find<FLaunchWindowModule>();
    if ( WindowModule != nullptr and WindowModule->GetWindow().IsValid() )
    {
        const auto Handles = WindowModule->GetWindow()->GetNativeHandles();
        if ( Handles )
        {
            const auto Converted = ToRenderSurface( *Handles );
            if ( Converted )
            {
                Surface = *Converted;
            }
        }
    }
    if ( not Surface.bPresent )
    {
        Surface.Source        = "Headless";
        Surface.bPresent      = false;
        Surface.InitialExtent = { 1280U, 720U, 1U };
    }

    FString Backend      = "auto";
    const auto Requested = InContext.GetCommandLine().GetValue( "renderer" );
    if ( Requested )
    {
        Backend = FString( *Requested );
    }

    TUniquePtr<FLaunchRendererModule> Module  = MakeUnique<FLaunchRendererModule>( MakeUnique<FRenderer>(), Surface, Backend );
    FLaunchApplicationModule *const AppModule = InContext.Find<FLaunchApplicationModule>();
    if ( AppModule != nullptr )
    {
        Module->InstallResizeHandler( *AppModule );
    }
    return Module;
}

Lumen::Launch::FLaunchRendererModule::FLaunchRendererModule ( TUniquePtr<FRenderer> InRenderer,
                                                              FRenderSurfaceDesc InSurface,
                                                              FString InBackend ) noexcept
    : Renderer( std::move( InRenderer ) ), Surface( std::move( InSurface ) ), Backend( std::move( InBackend ) )
{
}

void Lumen::Launch::FLaunchRendererModule::InstallResizeHandler ( FLaunchApplicationModule &InAppModule ) noexcept
{
    ResizeHandler.Emplace( InAppModule.PushMessageHandler<FLaunchRendererResizeHandler>( *this ) );
}

Lumen::Launch::FLaunchRendererModule::~FLaunchRendererModule () noexcept
{
    /** INFO: LIFO: the module's handler is popped here, never from a client session. */
    ResizeHandler.Reset();
}

Lumen::Launch::EModuleTraits Lumen::Launch::FLaunchRendererModule::GetTraits () const noexcept
{
    return EModuleTraits::None;
}

Lumen::FRenderer &Lumen::Launch::FLaunchRendererModule::GetRenderer () noexcept
{
    return *Renderer;
}

const Lumen::FRenderSurfaceDesc &Lumen::Launch::FLaunchRendererModule::GetSurfaceDesc () const noexcept
{
    return Surface;
}

const Lumen::FString &Lumen::Launch::FLaunchRendererModule::GetDeviceDescription () const noexcept
{
    const FString &DeviceName = Renderer->GetDeviceName();
    return DeviceName.empty() ? Surface.Source : DeviceName;
}

Lumen::TExpected<void, Lumen::ERendererError>
Lumen::Launch::FLaunchRendererModule::StartSession ( const FRenderSessionDesc &InDesc ) noexcept
{
    if ( bRendererStopped )
    {
        Renderer         = MakeUnique<FRenderer>();
        bRendererStopped = false;
    }
    const FRendererDesc Desc{ .SnapshotStream  = InDesc.SnapshotStream,
                              .Extent          = InDesc.Extent,
                              .Surface         = Surface,
                              .Backend         = Backend,
                              .CommandCapacity = 8ULL,
                              .Shaders         = {},
                              .ProxyCapacity   = 1ULL,
                              .AssetCapacity   = 1ULL,
                              .Readback        = InDesc.Readback,
                              .Feature         = InDesc.Feature };
    return Renderer->Start( Desc );
}

Lumen::Launch::FScopedRenderSession::FScopedRenderSession ( FLaunchRendererModule &InModule,
                                                            const FRenderSessionDesc &InDesc ) noexcept
    : Module( &InModule )
{
    const auto Started = Module->StartSession( InDesc );
    if ( not Started )
    {
        LUMEN_LOG_ERROR( LogLaunchRenderer, "Renderer session failed to start with error {}.",
                         static_cast<UInt32>( Started.Error() ) );
    }
}

Lumen::Launch::FScopedRenderSession::~FScopedRenderSession () noexcept
{
    if ( Module == nullptr )
    {
        return;
    }
    Module->GetRenderer().RequestShutdown();
    Module->GetRenderer().Join();
    Module->bRendererStopped = true;
    Module                   = nullptr;
}

Lumen::Bool Lumen::Launch::FScopedRenderSession::IsActive () const noexcept
{
    return Module != nullptr and Module->GetRenderer().IsRunning();
}

void Lumen::Launch::FScopedRenderSession::RequestRender ( const UInt64 InFrameIndex ) noexcept
{
    if ( Module != nullptr )
    {
        const auto Queued = Module->GetRenderer().RequestRender( InFrameIndex );
        if ( not Queued and Queued.Error() != ERendererError::QueueFull )
        {
            LUMEN_LOG_WARNING( LogLaunchRenderer, "Render request {} rejected with error {}.", InFrameIndex,
                               static_cast<UInt32>( Queued.Error() ) );
        }
    }
}

void Lumen::Launch::FScopedRenderSession::SwapStatusBuffers () noexcept
{
    if ( Module != nullptr )
    {
        Module->GetRenderer().SwapStatusBuffers();
    }
}

const Lumen::FRenderFrameStatus &Lumen::Launch::FScopedRenderSession::GetStatus () const noexcept
{
    return Module->GetRenderer().GetStatus();
}
