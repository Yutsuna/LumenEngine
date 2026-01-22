/**
 * @file LinuxWindow.hpp
 * @brief Declaration of the FLinuxWindow class for Linux-specific window management.
 */

#pragma once

#include "Generic/GenericWindow.hpp"

#include <SDL3/SDL.h>

namespace LumenEngine
{

class FLinuxApplication;

class FLinuxWindow : public FGenericWindow
{
public:

    ~FLinuxWindow () override;

    static TSharedRef<FLinuxWindow> Make ();

    void Initialize ( FLinuxApplication *const Application, const TSharedRef<FGenericWindowDescription> &InDescription, const TSharedPtr<FGenericWindow> &InParentWindow, const bool bShowImmediately );

    SDL_Window *GetOSWindowHandle () const;

    void Show () override;
    void Hide () override;

private:

    FLinuxWindow ();

private:

    FLinuxApplication         *LinuxApplication;
    SDL_Window                *WindowHandle;
    TSharedPtr<FGenericWindow> ParentWindow;
};

} // namespace LumenEngine
