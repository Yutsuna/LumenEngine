/**
 * @file LinuxWindow.cpp
 * @brief Implementation of the FLinuxWindow class for Linux-specific window management.
 */

#include "Linux/LinuxWindow.hpp"

#include "Logging/Logger.hpp"

/**
 * helpers
 */

namespace LumenEngine
{

namespace
{

    static LumenEngine::FLogCategory LogApplicationCore( "ApplicationCore" );

    static inline UInt32 GetSDLWindowFlags ( const TSharedRef<FGenericWindowDescription> &InDescription, const Bool bShowImmediately )
    {
        UInt32 WindowFlags = 0;

        if ( InDescription->bIsVisible && bShowImmediately )
        {
            WindowFlags |= SDL_WINDOW_RESIZABLE;
        }
        else
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
    if ( Renderer )
    {
        SDL_DestroyRenderer( Renderer );
    }
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

    if ( WindowHandle )
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

    if ( not WindowHandle )
    {
        LUMEN_LOG_ERROR( LogApplicationCore, "Failed to create SDL window: {}", SDL_GetError() );
        return;
    }

    SDL_SetWindowPosition( WindowHandle, InDescription->Position.X, InDescription->Position.Y );

    Renderer = SDL_CreateRenderer( WindowHandle, nullptr );
    if ( not Renderer )
    {
        LUMEN_LOG_ERROR( LogApplicationCore, "Failed to create SDL renderer: {}", SDL_GetError() );
    }
    /** INFO: Force a Clear() because some platforms (e.g., Linux) may not properly initialize the window's content until it's cleared at least once. */
    Clear();
}

void LumenEngine::FLinuxWindow::Show ()
{
    if ( WindowHandle )
    {
        SDL_ShowWindow( WindowHandle );
        Description->bIsVisible = true;
    }
}

void LumenEngine::FLinuxWindow::Hide ()
{
    if ( WindowHandle )
    {
        SDL_HideWindow( WindowHandle );
        Description->bIsVisible = false;
    }
}

void LumenEngine::FLinuxWindow::Clear ()
{
    if ( Renderer )
    {
        SDL_RenderClear( Renderer );
        SDL_RenderPresent( Renderer );
    }
}

/**
 * private
 */

LumenEngine::FLinuxWindow::FLinuxWindow () : WindowHandle( nullptr ), Renderer( nullptr ), FGenericWindow()
{
    /* Ctor */
}
