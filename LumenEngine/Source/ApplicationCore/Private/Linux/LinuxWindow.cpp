/**
 * @file LinuxWindow.cpp
 * @brief Implementation of the FLinuxWindow class for Linux-specific window management.
 */

#include "Linux/LinuxWindow.hpp"

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )

    #include "Logging/Logger.hpp"

/**
 * helpers
 */

namespace LumenEngine
{

namespace
{

    const LumenEngine::FLogCategory LogApplicationCore( "ApplicationCore" );

    inline UInt32 GetSDLWindowFlags ( const TSharedRef<FGenericWindowDescription> &InDescription, const Bool bShowImmediately )
    {
        UInt32 WindowFlags = SDL_WINDOW_VULKAN;

        if ( not( InDescription->bIsVisible && bShowImmediately ) )
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
            // TODO: Implement windowed fullscreen mode
            WindowFlags |= SDL_WINDOW_FULLSCREEN;
        }

        return WindowFlags;
    }

} // namespace

} // namespace LumenEngine

/**
 * public
 */

LumenEngine::FLinuxWindow::~FLinuxWindow ()
{
    if ( WindowHandle )
    {
        SDL_DestroyWindow( WindowHandle );
    }
}

LumenEngine::TSharedRef<LumenEngine::FLinuxWindow> LumenEngine::FLinuxWindow::Make ()
{
    return MakeShareable<FLinuxWindow>( new FLinuxWindow() );
}

SDL_Window *LumenEngine::FLinuxWindow::GetOSWindowHandle () const
{
    return WindowHandle;
}

SDL_WindowID LumenEngine::FLinuxWindow::GetOSWindowID () const
{
    static constexpr const SDL_WindowID InvalidWindowID = 0;

    if ( WindowHandle != nullptr )
    {
        return SDL_GetWindowID( WindowHandle );
    }
    return InvalidWindowID;
}

void LumenEngine::FLinuxWindow::Initialize ( FLinuxApplication *const Application,
                                             const TSharedRef<FGenericWindowDescription> &InDescription,
                                             const TSharedPtr<FGenericWindow> &InParentWindow,
                                             const bool bShowImmediately )
{
    Description      = InDescription;
    LinuxApplication = Application;
    ParentWindow     = InParentWindow;

    const UInt32 WindowFlags = GetSDLWindowFlags( InDescription, bShowImmediately );

    WindowHandle = SDL_CreateWindow( InDescription->Title.c_str(), InDescription->Size.Width, InDescription->Size.Height, WindowFlags );

    if ( WindowHandle == nullptr )
    {
        LUMEN_LOG_ERROR( LogApplicationCore, "Failed to create SDL window: {}", SDL_GetError() );
        return;
    }

    SDL_SetWindowPosition( WindowHandle, InDescription->Position.X, InDescription->Position.Y );
}

void LumenEngine::FLinuxWindow::Show ()
{
    if ( WindowHandle != nullptr )
    {
        SDL_ShowWindow( WindowHandle );
        Description->bIsVisible = true;
    }
}

void LumenEngine::FLinuxWindow::Hide ()
{
    if ( WindowHandle != nullptr )
    {
        SDL_HideWindow( WindowHandle );
        Description->bIsVisible = false;
    }
}

#endif
