/// @file LinuxWindow.hpp
/// @brief Declaration of the FLinuxWindow class for Linux-specific window management

#pragma once

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_LINUX )
    #include <Generic/GenericWindow.hpp>
    #include <SDL3/SDL.h>

namespace Lumen
{

class FLinuxApplication;

///
/// @class FLinuxWindow
/// @brief A class that represents a window in the Linux environment
///
class APPCORE_EXPORT FLinuxWindow : public FGenericWindow
{
public:

    ~FLinuxWindow () noexcept override;

public:

    [[nodiscard]] static TSharedRef<FLinuxWindow> Make ();

    /// @brief Creates the native SDL window and applies the given description.
    void Initialize ( FLinuxApplication *InApplication,
                      const TSharedRef<FGenericWindowDescription> &InDescription,
                      const TSharedPtr<FGenericWindow> &InParentWindow,
                      Bool bShowImmediately ) noexcept;

public:

    /// @brief Ask the OS to destroy OS-specific resource associated with the window
    void Destroy () noexcept override;

    /// @brief Shows the OS window on the screen
    void Show () noexcept override;

    /// @brief Minimize-to-taskbar the window
    void Minimize () noexcept override;

    /// @brief Maximize the window
    void Maximize () noexcept override;

    /// @brief Hide the window from the screen
    void Hide () noexcept override;

    /// @brief Toggle native window between fullscreen and normal mode
    void SetWindowMode ( EWindowMode::Type InWindowMode ) noexcept override;

    /// @return The current window mode
    [[nodiscard]] EWindowMode::Type GetWindowMode () const noexcept override;

    /// @return True if the window is maximized
    [[nodiscard]] Bool IsMaximized () const noexcept override;

    /// @return True if the window is minimized
    [[nodiscard]] Bool IsMinimized () const noexcept override;

    /// @brief Let the OS know that a window has taken focus
    void SetWindowFocus () noexcept override;

    /// @brief Sets the opacity of this window
    /// @param InOpacity The new opacity of the window as floating Point Scalar
    void SetOpacity ( Float32 InOpacity ) noexcept override;

    //// @brief Enables or disable  the window's ability to accept input
    /// @param bEnable true to enable input, false to disable it.
    void EnableInput ( Bool bEnable ) noexcept override;

    /// @brief Gets the OS Window border size
    [[nodiscard]] Int32 GetOSWindowBorderSize () const noexcept override;

    /// @brief Gets the  OS Window title bar size
    [[nodiscard]] Int32 GetOSWindowTitleBarSize () const noexcept override;

    /// @return True if the window is in foreground
    [[nodiscard]] Bool IsForegroundWindow () const noexcept override;

    /// @brief Sets the window text-title
    /// @param InText the new text-title of the window
    void SetWindowTitle ( const AnsiChar *InText ) noexcept override;

    /// @return The SDL Window ID
    [[nodiscard]] SDL_WindowID GetOSWindowID () const noexcept;

    /// @return The SDL Window Handle pointer
    [[nodiscard]] SDL_Window *GetOSWindowHandle () const noexcept;

private:

    FLinuxWindow () noexcept = default;

private:

    FLinuxApplication *LinuxApplication     = nullptr;
    SDL_Window *WindowHandle                = nullptr;
    TSharedPtr<FGenericWindow> ParentWindow = nullptr;
};

} // namespace Lumen

#endif
