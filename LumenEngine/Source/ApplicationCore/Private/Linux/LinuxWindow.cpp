/**
 * @file LinuxWindow.cpp
 * @brief Implementation of the FLinuxWindow class for Linux-specific window management.
 */

#include "Linux/LinuxWindow.hpp"

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

void LumenEngine::FLinuxWindow::Initialize ( FLinuxApplication *const Application, const TSharedRef<FGenericWindowDescription> &InDescription, const TSharedPtr<FGenericWindow> &InParentWindow, const bool bShowImmediately )
{
    Description      = InDescription;
    LinuxApplication = Application;
    ParentWindow     = InParentWindow;
}

/**
 * private
 */

LumenEngine::FLinuxWindow::FLinuxWindow ()
    : WindowHandle( nullptr ), FGenericWindow()
{
    /* __ctor__ */
}
