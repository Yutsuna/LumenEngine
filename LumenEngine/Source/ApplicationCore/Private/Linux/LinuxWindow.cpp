/**
 * @file LinuxWindow.cpp
 * @brief Implementation of the FLinuxWindow class for Linux-specific window management.
 */

#include "Linux/LinuxWindow.hpp"

/**
 * public
 */

LumenEngine::TSharedRef<LumenEngine::FLinuxWindow> LumenEngine::FLinuxWindow::Make ()
{
    return MakeShareable<FLinuxWindow>( new FLinuxWindow() );
}

SDL_Window *LumenEngine::FLinuxWindow::GetOSWindowHandle () const
{
    return WindowHandle;
}

/**
 * private
 */

LumenEngine::FLinuxWindow::FLinuxWindow ()
    : WindowHandle( nullptr ), FGenericWindow()
{
    /* __ctor__ */
}
