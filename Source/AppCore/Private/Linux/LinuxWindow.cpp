/**
 * @file LinuxWindow.cpp
 * @brief Implementation of the FLinuxWindow class for Linux-specific window management.
 */

#include <Linux/LinuxWindow.hpp>

#if defined( LUMEN_PLATFORM_LINUX )

    #include <Logging/Logger.hpp>
    #include <String/StringView.hpp>

    #include <Templates/Unexpected.hpp>

    #include <SDL3/SDL_video.h>

namespace
{

const Lumen::FLogCategory LogLinuxWindow( "LinuxWindow" );

inline SDL_WindowFlags GetSDLWindowFlags ( const Lumen::TSharedRef<Lumen::FGenericWindowDescription> &InDescription,
                                           const Lumen::Bool bShowImmediately,
                                           const SDL_WindowFlags InGraphicsFlags = static_cast<SDL_WindowFlags>( 0 ) ) noexcept
{
    SDL_WindowFlags WindowFlags = InGraphicsFlags;

    if ( not( InDescription->bIsVisible and bShowImmediately ) )
    {
        WindowFlags = static_cast<SDL_WindowFlags>( WindowFlags | SDL_WINDOW_HIDDEN );
    }

    if ( InDescription->bIsResizable )
    {
        WindowFlags = static_cast<SDL_WindowFlags>( WindowFlags | SDL_WINDOW_RESIZABLE );
    }

    switch ( InDescription->WindowMode )
    {
    case Lumen::EWindowMode::Windowed:
    {
        /** NOTE: Default SDL window mode. No additional flag required. */
        break;
    }

    case Lumen::EWindowMode::Fullscreen: /** NOLINT(bugprone-branch-clone) */
    {
        WindowFlags = static_cast<SDL_WindowFlags>( WindowFlags | SDL_WINDOW_FULLSCREEN );
        break;
    }

    case Lumen::EWindowMode::WindowedFullscreen:
    {
        /** INFO: SDL3 dropped SDL_WINDOW_FULLSCREEN_DESKTOP; a borderless window is the equivalent mode. */
        WindowFlags = static_cast<SDL_WindowFlags>( WindowFlags | SDL_WINDOW_BORDERLESS );
        break;
    }

    default:
        break;
    }

    return WindowFlags;
}

} // namespace

/** INFO: FLinuxWindow lifetime implementation. */

Lumen::FLinuxWindow::~FLinuxWindow () noexcept
{
    this->FLinuxWindow::Destroy();
}

/** INFO: Factory for Linux-backed windows. */

Lumen::TSharedRef<Lumen::FLinuxWindow> Lumen::FLinuxWindow::Make () noexcept
{
    /** NOLINTNEXTLINE(bugprone-unhandled-exception-at-new) */
    return MakeShareable<FLinuxWindow>( new FLinuxWindow() );
}

Lumen::TExpected<void, Lumen::EAppCoreError::Type>
Lumen::FLinuxWindow::Initialize ( FLinuxApplication *InApplication,
                                  const TSharedRef<FGenericWindowDescription> &InDescription,
                                  const TSharedPtr<FGenericWindow> &InParentWindow,
                                  const Bool bShowImmediately ) noexcept
{
    /** TODO: make this parametrable */
    const SDL_WindowFlags WindowBackendAPI = SDL_WINDOW_VULKAN;
    const SDL_WindowFlags WindowFlags      = GetSDLWindowFlags( InDescription, bShowImmediately, WindowBackendAPI );

    WindowHandle = SDL_CreateWindow( InDescription->Title.c_str(), InDescription->Size.X, InDescription->Size.Y, WindowFlags );

    if ( WindowHandle == nullptr )
    {
        LUMEN_LOG_ERROR( LogLinuxWindow, "Failed to create SDL window: {}", SDL_GetError() );
        return MakeUnexpected( EAppCoreError::Type::WindowCreationFailed );
    }

    LinuxApplication = InApplication;
    Description      = InDescription;
    ParentWindow     = InParentWindow;

    SDL_SetWindowPosition( WindowHandle, InDescription->Position.X, InDescription->Position.Y );
    return {};
}

/** INFO: FGenericWindow overrides. */

void Lumen::FLinuxWindow::Destroy () noexcept
{
    if ( WindowHandle != nullptr )
    {
        SDL_DestroyWindow( WindowHandle );
        WindowHandle = nullptr;
    }
}

void Lumen::FLinuxWindow::Show () noexcept
{
    if ( WindowHandle != nullptr )
    {
        SDL_ShowWindow( WindowHandle );
        Description->bIsVisible = true;
    }
}

void Lumen::FLinuxWindow::Minimize () noexcept
{
    if ( WindowHandle != nullptr )
    {
        SDL_MinimizeWindow( WindowHandle );
        Description->bIsVisible = false;
    }
}

void Lumen::FLinuxWindow::Maximize () noexcept
{
    if ( WindowHandle != nullptr )
    {
        SDL_MaximizeWindow( WindowHandle );
        Description->bIsVisible = true;
    }
}

void Lumen::FLinuxWindow::Hide () noexcept
{
    if ( WindowHandle != nullptr )
    {
        SDL_HideWindow( WindowHandle );
        Description->bIsVisible = false;
    }
}

void Lumen::FLinuxWindow::SetWindowMode ( EWindowMode::Type LUMEN_UNUSED InWindowMode ) noexcept
{
    /** TODO: Apply the requested window mode through SDL. */
}

Lumen::EWindowMode::Type Lumen::FLinuxWindow::GetWindowMode () const noexcept
{
    /** TODO: Query the current mode from the SDL window. */
    return EWindowMode::Windowed;
}

Lumen::Bool Lumen::FLinuxWindow::IsMaximized () const noexcept
{
    /** TODO: Query the maximized state from the SDL window. */
    return false;
}

Lumen::Bool Lumen::FLinuxWindow::IsMinimized () const noexcept
{
    /** TODO: Query the minimized state from the SDL window. */
    return false;
}

void Lumen::FLinuxWindow::SetWindowFocus () noexcept
{
    /** TODO: Request OS focus for the SDL window. */
}

void Lumen::FLinuxWindow::SetOpacity ( Float32 LUMEN_UNUSED InOpacity ) noexcept
{
    /** TODO: Apply opacity through SDL. */
}

void Lumen::FLinuxWindow::EnableInput ( Bool LUMEN_UNUSED bEnable ) noexcept
{
    /** TODO: Toggle SDL window input acceptance. */
}

Lumen::Int32 Lumen::FLinuxWindow::GetOSWindowBorderSize () const noexcept
{
    /** TODO: Query the native border size. */
    return 0;
}

Lumen::Int32 Lumen::FLinuxWindow::GetOSWindowTitleBarSize () const noexcept
{
    /** TODO: Query the native title bar size. */
    return 0;
}

SDL_WindowID Lumen::FLinuxWindow::GetOSWindowID () const noexcept
{
    static constexpr SDL_WindowID InvalidWindowID = 0;

    if ( WindowHandle != nullptr )
    {
        return SDL_GetWindowID( WindowHandle );
    }
    return InvalidWindowID;
}

SDL_Window *Lumen::FLinuxWindow::GetOSWindowHandle () const noexcept
{
    return WindowHandle;
}

Lumen::TOptional<Lumen::FNativeWindowHandles> Lumen::FLinuxWindow::GetNativeHandles () const noexcept
{
    if ( WindowHandle == nullptr )
    {
        return {};
    }

    const AnsiChar *const DriverName  = SDL_GetCurrentVideoDriver();
    const SDL_PropertiesID Properties = SDL_GetWindowProperties( WindowHandle );
    if ( DriverName == nullptr or Properties == 0 )
    {
        return {};
    }

    Int32 PixelWidth  = 0;
    Int32 PixelHeight = 0;
    if ( not SDL_GetWindowSizeInPixels( WindowHandle, &PixelWidth, &PixelHeight ) or PixelWidth <= 0 or PixelHeight <= 0 )
    {
        return {};
    }

    FNativeWindowHandles Handles;
    Handles.Width  = static_cast<UInt32>( PixelWidth );
    Handles.Height = static_cast<UInt32>( PixelHeight );

    const FStringView Driver( DriverName );
    if ( Driver == "x11" )
    {
        const Int64 NativeWindow =
            static_cast<Int64>( SDL_GetNumberProperty( Properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0 ) );
        Handles.NativeDisplay =
            reinterpret_cast<UPtr>( SDL_GetPointerProperty( Properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr ) );
        if ( NativeWindow <= 0 or Handles.NativeDisplay == 0U )
        {
            return {};
        }
        Handles.WindowSystem = ENativeWindowSystem::X11;
        Handles.NativeWindow = static_cast<UPtr>( NativeWindow );
        return TOptional<FNativeWindowHandles>( Handles );
    }

    if ( Driver == "wayland" )
    {
        Handles.NativeDisplay =
            reinterpret_cast<UPtr>( SDL_GetPointerProperty( Properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr ) );
        Handles.NativeWindow =
            reinterpret_cast<UPtr>( SDL_GetPointerProperty( Properties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr ) );
        Handles.NativeWindowAttachment =
            reinterpret_cast<UPtr>( SDL_GetPointerProperty( Properties, SDL_PROP_WINDOW_WAYLAND_EGL_WINDOW_POINTER, nullptr ) );
        if ( Handles.NativeWindow == 0U or Handles.NativeDisplay == 0U )
        {
            return {};
        }
        Handles.WindowSystem = ENativeWindowSystem::Wayland;
        return TOptional<FNativeWindowHandles>( Handles );
    }

    return {};
}

Lumen::Bool Lumen::FLinuxWindow::IsForegroundWindow () const noexcept
{
    /** TODO: Query the foreground state from the SDL window. */
    return false;
}

void Lumen::FLinuxWindow::SetWindowTitle ( LUMEN_UNUSED const AnsiChar *const InText ) noexcept
{
    /** TODO: Set the SDL window title. */
}

#endif
