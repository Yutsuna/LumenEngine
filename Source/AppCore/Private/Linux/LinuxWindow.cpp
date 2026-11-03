/// @file LinuxWindow.cpp
/// @brief Implementation of the FLinuxWindow class for Linux-specific window management.

#include <Linux/LinuxWindow.hpp>

#if defined( LUMEN_PLATFORM_LINUX )

    #include <Logging/Logger.hpp>

namespace
{

const Lumen::FLogCategory LogLinuxWindow( "LinuxWindow" );

inline SDL_WindowFlags GetSDLWindowFlags( const TSharedRef<FGenericWindowDescription> &InDescription, const Lumen::Bool bShowImmediately ) noexcept
{
    /// TODO: check if this is OK because the backend might not always be Vulkan
    SDL_WindowFlags WindowFlags = SDL_WINDOW_VULKAN;

    if ( not( InDescription->bIsVisible and bShowImmediately ) )
    {
        WindowFlags |= SDL_WINDOW_HIDDEN;
    }

    if ( InDescription->bIsResizable )
    {
        WindowFlags |= SDL_WINDOW_RESIZABLE;
    }

    if ( InDescription->WindowMode == EWindowMode::Fullscreen )
    {
        WindowFlags |= SDL_WINDOW_FULLSCREEN;
    }
    else if ( InDescription->WindowMode == EWindowMode::WindowedFullscreen )
    {
        /// TODO: Implement windowed fullscreen mode
        WindowFlags |= SDL_WINDOW_FULLSCREEN;
    }

    return WindowFlags;

}

} // namespace

///
/// Public
///

Lumen::FLinuxWindow::~FLinuxWindow () noexcept
{
    this->FLinuxWindow::Destroy();
}

///
/// Static
///

Lumen::TSharedRef<Lumen::FLinuxWindow> Lumen::FLinuxWindow::Make ()
{
    return MakeShareable<FLinuxWindow>( new FLinuxWindow() );
}

void Lumen::FLinuxWindow::Initialize ( FLinuxApplication *InApplication,
                                       const TSharedRef<FGenericWindowDescription> &InDescription,
                                       const TSharedPtr<FGenericWindow> &InParentWindow,
                                       const Bool bShowImmediately ) noexcept
{
    LinuxApplication = InApplication;
    Description      = InDescription;
    ParentWindow     = InParentWindow;

    const SDL_WindowFlags WindowFlags = GetSDLWindowFlags( InDescription, bShowImmediately );

    WindowHandle = SDL_CreateWindow( InDescription->Title.c_str(), InDescription->Size.X, InDescription->Size.Y, WindowFlags );

    if ( WindowHandle == nullptr )
    {
        LUMEN_LOG_ERROR( LogLinuxWindow, "Failed to create SDL window: {}", SDL_GetError() );
        return;
    }

    SDL_SetWindowPosition( WindowHandle, InDescription->Position.X, InDescription->Position.Y );
}

///
/// Public
///

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
    /// TODO
}

Lumen::EWindowMode::Type Lumen::FLinuxWindow::GetWindowMode () const noexcept
{
    /// TODO
    return EWindowMode::Windowed;
}

Lumen::Bool Lumen::FLinuxWindow::IsMaximized () const noexcept
{
    /// TODO
    return false;
}

Lumen::Bool Lumen::FLinuxWindow::IsMinimized () const noexcept
{
    /// TODO
    return false;
}

void Lumen::FLinuxWindow::SetWindowFocus () noexcept
{
    /// TODO
}

void Lumen::FLinuxWindow::SetOpacity ( Float32 LUMEN_UNUSED InOpacity ) noexcept
{
    /// TODO
}

void Lumen::FLinuxWindow::EnableInput ( Bool LUMEN_UNUSED bEnable ) noexcept
{
    /// TODO
}

Lumen::Int32 Lumen::FLinuxWindow::GetOSWindowBorderSize () const noexcept
{
    /// TODO
    return 0;
}

Lumen::Int32 Lumen::FLinuxWindow::GetOSWindowTitleBarSize () const noexcept
{
    /// TODO
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

Lumen::Bool Lumen::FLinuxWindow::IsForegroundWindow () const noexcept
{
    /// TODO
    return false;
}

void Lumen::FLinuxWindow::SetWindowTitle ( LUMEN_UNUSED const AnsiChar *const InText ) noexcept
{
    /// TODO
}

#endif
