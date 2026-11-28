/**
 * @file WindowsWindow.hpp
 * @brief Declaration of the FWindowsWindow class for Windows-specific window management.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_WINDOWS )

    #include <AppCoreExport.hpp>
    #include <Generic/AppCoreError.hpp>
    #include <Generic/GenericWindow.hpp>

    #include <Templates/Expected.hpp>

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>

namespace Lumen
{

class FWindowsApplication;

/**
 * @class FWindowsWindow
 * @brief A class that represents a native Win32 window on Windows.
 */
class APPCORE_EXPORT FWindowsWindow : public FGenericWindow
{
public:

    ~FWindowsWindow () noexcept override;

public:

    /** @brief Factory that creates a Windows window instance. */
    [[nodiscard]] static TSharedRef<FWindowsWindow> Make () noexcept;

    /**
     * @brief Creates the native Win32 window and applies the given description.
     * @return Success, or EAppCoreError::Type::WindowCreationFailed if Win32 refused to create the window.
     */
    [[nodiscard]] TExpected<void, EAppCoreError::Type> Initialize ( FWindowsApplication *InApplication,
                                                                    const TSharedRef<FGenericWindowDescription> &InDescription,
                                                                    const TSharedPtr<FGenericWindow> &InParentWindow,
                                                                    Bool bShowImmediately ) noexcept;

public:

    /** @brief Ask the OS to destroy the Win32 HWND associated with the window. */
    void Destroy () noexcept override;

    /** @brief Shows the OS window on the screen. */
    void Show () noexcept override;

    /** @brief Minimize-to-taskbar the window. */
    void Minimize () noexcept override;

    /** @brief Maximize the window. */
    void Maximize () noexcept override;

    /** @brief Hide the window from the screen. */
    void Hide () noexcept override;

    /** @brief Toggle native window between fullscreen, borderless fullscreen and windowed modes. */
    void SetWindowMode ( EWindowMode::Type InWindowMode ) noexcept override;

    /** @return The current window mode. */
    [[nodiscard]] EWindowMode::Type GetWindowMode () const noexcept override;

    /** @return True if the window is maximized. */
    [[nodiscard]] Bool IsMaximized () const noexcept override;

    /** @return True if the window is minimized. */
    [[nodiscard]] Bool IsMinimized () const noexcept override;

    /** @return True if the window is visible on screen. */
    [[nodiscard]] Bool IsVisible () const noexcept override;

    /** @brief Let the OS know that a window has taken focus. */
    void SetWindowFocus () noexcept override;

    /**
     * @brief Sets the opacity of this window.
     * @param InOpacity The new opacity of the window in [0.0, 1.0].
     */
    void SetOpacity ( Float32 InOpacity ) noexcept override;

    /**
     * @brief Enables or disables the window's ability to accept input.
     * @param bEnable True to enable input, false to disable it.
     */
    void EnableInput ( Bool bEnable ) noexcept override;

    /** @brief Gets the OS Window border size in pixels. */
    [[nodiscard]] Int32 GetOSWindowBorderSize () const noexcept override;

    /** @brief Gets the OS Window title bar size in pixels. */
    [[nodiscard]] Int32 GetOSWindowTitleBarSize () const noexcept override;

    /** @return True if the window is in foreground. */
    [[nodiscard]] Bool IsForegroundWindow () const noexcept override;

    /**
     * @brief Sets the window text-title (UTF-8).
     * @param InText The new text-title of the window.
     */
    void SetWindowTitle ( const AnsiChar *InText ) noexcept override;

    /** @brief Gets the OS Window client width in pixels. */
    [[nodiscard]] Int32 GetWindowWidth () const noexcept override;

    /** @brief Gets the OS Window client height in pixels. */
    [[nodiscard]] Int32 GetWindowHeight () const noexcept override;

    /** @return The current client drawable size in pixels. */
    [[nodiscard]] Maths::FVec2i GetWindowSize () const noexcept override;

    /** @return The current window position in screen coordinates. */
    [[nodiscard]] Maths::FVec2i GetWindowPosition () const noexcept override;

    /** @return Non-owning Win32 handles (HWND in NativeWindow, HINSTANCE in NativeDisplay). */
    [[nodiscard]] TOptional<FNativeWindowHandles> GetNativeHandles () const noexcept override;

public:

    /** @return The Win32 HWND handle. */
    [[nodiscard]] HWND GetOSWindowHandle () const noexcept;

    /** @return The Win32 HINSTANCE handle owning this window. */
    [[nodiscard]] HINSTANCE GetOSInstanceHandle () const noexcept;

    /** @return The wide Win32 window class name shared by Lumen windows. */
    [[nodiscard]] static const Lumen::WideChar *GetWindowClassName () noexcept;

private:

    friend class FWindowsApplication;

    FWindowsWindow () noexcept = default;

private:

    FWindowsApplication *WindowsApplication = nullptr;
    HWND WindowHandle                       = nullptr;
    HINSTANCE InstanceHandle                = nullptr;
    TSharedPtr<FGenericWindow> ParentWindow = nullptr;
    EWindowMode::Type CurrentWindowMode     = EWindowMode::Windowed;
    WINDOWPLACEMENT SavedWindowPlacement{};
    DWORD SavedWindowStyle   = 0U;
    DWORD SavedWindowExStyle = 0U;
    Bool bHasSavedPlacement  = false;
};

} // namespace Lumen

#endif
