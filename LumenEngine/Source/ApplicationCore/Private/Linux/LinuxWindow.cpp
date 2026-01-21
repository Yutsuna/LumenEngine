/**
 * @file LinuxWindow.cpp
 * @brief Implementation of the FLinuxWindow class for Linux-specific window management.
 */

#include "Linux/LinuxWindow.hpp"

std::shared_ptr<LumenEngine::FLinuxWindow> LumenEngine::FLinuxWindow::Make ()
{
    return std::make_shared<FLinuxWindow>();
}

/**
 * private
 */

LumenEngine::FLinuxWindow::FLinuxWindow ()
    : WindowHandle( nullptr ), bIsVisible( false ), bIsFullScreen( false )
{
    /* __ctor__ */
}
