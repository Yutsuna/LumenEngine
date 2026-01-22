/**
 * @file LinuxWindow.cpp
 * @brief Implementation of the FLinuxWindow class for Linux-specific window management.
 */

#include "Linux/LinuxWindow.hpp"

LumenEngine::TSharedRef<LumenEngine::FLinuxWindow> LumenEngine::FLinuxWindow::Make ()
{
    return MakeShareable<FLinuxWindow>( new FLinuxWindow() );
}

/**
 * private
 */

LumenEngine::FLinuxWindow::FLinuxWindow ()
    : WindowHandle( nullptr )
{
    /* __ctor__ */
}
