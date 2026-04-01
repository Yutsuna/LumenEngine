
/**
 * @file LinuxWindow.hpp
 * @brief Declaration of the FLinuxWindow class for Linux-specific window management.
 */

#pragma once

#include "Definitions.hpp"

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )

    #include "Generic/GenericWindow.hpp"

    #include <SDL3/SDL.h>

namespace LumenEngine
{

class FLinuxApplication;

/**
 * @class FLinuxWindow
 * @brief A class that represents a window in the Linux environment, utilizing SDL for window management.
 */
class LUMEN_ENGINE_API FLinuxWindow : public FGenericWindow
{

public:

    ~FLinuxWindow () override;

public:

    [[nodiscard]] static TSharedRef<FLinuxWindow> Make ();

public:

    /**
     * @brief Initializes the window with the specified parameters.
     * @param Application The Linux application instance that owns this window.
     * @param InDescription A shared reference to the window description containing properties such as size, title, etc.
     * @param InParentWindow A shared pointer to the parent window, if any. This can be null for top-level windows.
     * @param bShowImmediately A boolean flag indicating whether the window should be shown immediately after initialization.
     */
    void Initialize ( FLinuxApplication *const Application,
                      const TSharedRef<FGenericWindowDescription> &InDescription,
                      const TSharedPtr<FGenericWindow> &InParentWindow,
                      const bool bShowImmediately );

    [[nodiscard]] SDL_Window *GetOSWindowHandle () const;
    [[nodiscard]] SDL_WindowID GetOSWindowID () const;

    /** @brief Shows the window on the screen. */
    void Show () override;

    /** @brief Hides the window from the screen. */
    void Hide () override;

private:

    FLinuxWindow () noexcept = default;

private:

    FLinuxApplication *LinuxApplication     = nullptr;
    SDL_Window *WindowHandle                = nullptr;
    TSharedPtr<FGenericWindow> ParentWindow = nullptr;
};

} // namespace LumenEngine

#endif
