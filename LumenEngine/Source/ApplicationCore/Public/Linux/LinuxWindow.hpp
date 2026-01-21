/**
 * @file LinuxWindow.hpp
 * @brief Declaration of the FLinuxWindow class for Linux-specific window management.
 */

#pragma once

#include "Generic/GenericWindow.hpp"
#include <SDL3/SDL.h>

#include <memory>

namespace LumenEngine
{

class FLinuxWindow : public FGenericWindow
{
public:

    ~FLinuxWindow () override;

    static std::shared_ptr<FLinuxWindow> Make ();

private:

    FLinuxWindow ();

private:

    SDL_Window *WindowHandle;
};

} // namespace LumenEngine
