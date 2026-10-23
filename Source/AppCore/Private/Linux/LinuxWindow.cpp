/// @file LinuxWindow.cpp
/// @brief Implementation of the FLinuxWindow class for Linux-specific window management.

#include <Linux/LinuxWindow.hpp>

#if defined( LUMEN_PLATFORM_LINUX )

    #include <Logging/Logger.hpp>

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

///
/// Public
///

void Lumen::FLinuxWindow::Clear () noexcept
{
}

void Lumen::FLinuxWindow::Destroy () noexcept
{
    if ( WindowHandle != nullptr )
    {
        SDL_DestroyWindow( WindowHandle );
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
}

void Lumen::FLinuxWindow::Maximize () noexcept
{
}

void Lumen::FLinuxWindow::Hide () noexcept
{
    if ( WindowHandle != nullptr )
    {
        SDL_ShowWindow( WindowHandle );
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
