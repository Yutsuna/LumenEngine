/**
 * @file GenericWindow.hpp
 * @brief Declaration of the FGenericWindow class for cross-platform window management.
 */

#pragma once

#include <AppCoreExport.hpp>

#include <Generic/GenericNativeWindowHandles.hpp>
#include <Generic/GenericWindowDescription.hpp>

#include <Memory/SharedPtr.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

/**
 * @class FGenericWindow
 * @brief Platform-independent interface for an OS window.
 */
class APPCORE_EXPORT FGenericWindow
{

public:

    FGenericWindow () noexcept          = default;
    virtual ~FGenericWindow () noexcept = default;

    /** @brief Clear the window's content */
    virtual void Clear () noexcept;

    /** @brief Ask the OS to destroy OS-specific resource associated with the window */
    virtual void Destroy () noexcept;

    /** @brief Shows the OS window on the screen */
    virtual void Show () noexcept;

    /** @brief Minimize-to-taskbar the window */
    virtual void Minimize () noexcept;

    /** @brief Maximize the window */
    virtual void Maximize () noexcept;

    /** @brief Hide the window from the screen */
    virtual void Hide () noexcept;

    /** @brief Toggle native window between fullscreen and normal mode */
    virtual void SetWindowMode ( EWindowMode::Type InWindowMode ) noexcept;

    /** @return The current window mode */
    [[nodiscard]] virtual EWindowMode::Type GetWindowMode () const noexcept;

    /** @return True if the window is maximized */
    [[nodiscard]] virtual Bool IsMaximized () const noexcept;

    /** @return True if the window is minimized */
    [[nodiscard]] virtual Bool IsMinimized () const noexcept;

    /** @return True if the window is visible on screen */
    [[nodiscard]] virtual Bool IsVisible () const noexcept;

    /** @brief Let the OS know that a window has taken focus */
    virtual void SetWindowFocus () noexcept;

    /**
     * @brief Sets the opacity of this window
     * @param InOpacity The new opacity of the window as floating Point Scalar
     */
    virtual void SetOpacity ( Float32 InOpacity ) noexcept;

    /**
     * @brief Enables or disables the window's ability to accept input
     * @param bEnable true to enable input, false to disable it.
     */
    virtual void EnableInput ( Bool bEnable ) noexcept;

    /** @return True if the native window exists underneath the coordinates */
    [[nodiscard]] virtual Bool IsPointInWindow ( Int32 InX, Int32 InY ) const noexcept;

    /** @brief Gets the OS Window border size */
    [[nodiscard]] virtual Int32 GetOSWindowBorderSize () const noexcept;

    /** @brief Gets the  OS Window title bar size */
    [[nodiscard]] virtual Int32 GetOSWindowTitleBarSize () const noexcept;

    /** @return True if the window is in foreground */
    [[nodiscard]] virtual Bool IsForegroundWindow () const noexcept;

    /**
     * @brief Sets the window text-title
     * @param InText the new text-title of the window
     */
    virtual void SetWindowTitle ( const AnsiChar *InText ) noexcept;

    /** @brief Gets the OS Window width */
    [[nodiscard]] virtual Int32 GetWindowWidth () const noexcept;

    /** @brief Gets the OS Window height */
    [[nodiscard]] virtual Int32 GetWindowHeight () const noexcept;

    /** @brief Gets the OS Window size */
    [[nodiscard]] virtual Maths::FVec2i GetWindowSize () const noexcept;

    /** @brief Gets the OS Window position */
    [[nodiscard]] virtual Maths::FVec2i GetWindowPosition () const noexcept;

    /**
     * @brief Returns non-owning native handles for composition with a rendering backend.
     * @details Must be called on the main thread. The returned value never extends the native
     *          window's lifetime and must be queried again after platform window transitions.
     * @return Native handles, or empty when the window or its window system cannot provide them.
     */
    [[nodiscard]] virtual TOptional<FNativeWindowHandles> GetNativeHandles () const noexcept;

protected:

    TSharedPtr<FGenericWindowDescription> Description;
};

} // namespace Lumen
