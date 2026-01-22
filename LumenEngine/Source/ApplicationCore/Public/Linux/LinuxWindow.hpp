/**
 * @file LinuxWindow.hpp
 * @brief Declaration of the FLinuxWindow class for Linux-specific window management.
 */

#pragma once

#include "Generic/GenericWindow.hpp"

#include <SDL3/SDL.h>

namespace LumenEngine
{

class FLinuxWindow : public FGenericWindow
{
public:

    ~FLinuxWindow () override;

    static TSharedRef<FLinuxWindow> Make ();

private:

    FLinuxWindow ();

private:

    SDL_Window *WindowHandle;
};

} // namespace LumenEngine
