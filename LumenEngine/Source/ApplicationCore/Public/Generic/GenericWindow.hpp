/**
 * @file GenericWindow.hpp
 * @brief Declaration of the GenericWindow class for cross-platform window management.
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace EWindowMode
{

    enum Type : Int32
    {
        /** True fullscreen window mode */
        Fullscreen,
        /** Borderless windowed fullscreen mode */
        WindowedFullscreen,
        /** Standard windowed mode */
        Windowed,

        /** Number of window modes */
        NumWindowModes
    };

    static inline const char *ToString ( const Type InWindowMode )
    {
        switch ( InWindowMode )
        {
        case Fullscreen:
            return "Fullscreen";
        case WindowedFullscreen:
            return "WindowedFullscreen";
        case Windowed:
            return "Windowed";
        default:
            return "Unknown";
        }
    }

} // namespace EWindowMode

class FGenericWindow
{
public:

    FGenericWindow ();
    virtual ~FGenericWindow ();

    /** Ask the OS to destroy OS-specific resource associated with the window */
    virtual void Destroy ();

    /** Minimize-to-taskbar the window */
    virtual void Minimize ();

    /** Maximize the window */
    virtual void Maximize ();

    /** Restore the window to its previous size and position */
    virtual void Restore ();

    /** Make the window visible on screen */
    virtual void Show ();

    /** Hide the window from the screen */
    virtual void Hide ();

    /** Toogle native window between fullscreen and normal mode */
    virtual void SetWindowMode ( const EWindowMode::Type InWindowMode );

    /** Get current window mode */
    virtual EWindowMode::Type GetWindowMode () const;

    /** @return true if the window is maximized */
    virtual Bool IsMaximized () const;

    /** @return true if the window is minimized to taskbar */
    virtual Bool IsMinimized () const;

    /** @return true if the window is visible on screen */
    virtual Bool IsVisible () const;

    /** Implement SetWindowFocus to let the OS know that a window has taken focus */
    virtual void SetWindowFocus ();

    /**
     * Sets the opacity of this window
     *
     * @param InOpacity The new opacity of the window represented as a floating point scalar
     *
     */
    virtual void SetOpacity ( const Float32 InOpacity );

    /**
     * Enables or disable the window's ability to accept input
     *
     * @param bEnable true to enable input, false to disable it
     */
    virtual void EnableInput ( const Bool bEnable );

    /** @return true if native window exists underneath the coordinates */
    virtual Bool IsPointInWindow ( const Int32 InX, const Int32 InY ) const;

    /** Gets the OS Window border size */
    virtual Int32 GetOSWindowBorderSize () const;

    /** Gets the OS Window title bar size */
    virtual Int32 GetOSWindowTitleBarSize () const;

    /** Gets the OS Window handle in the form of a void pointer for other API's to use */
    virtual void *GetOSWindowHandle () const;

    /** @return true if the window is in forgeground */
    virtual Bool IsForegroundWindow () const;

    /**
     * Sets the window text-title
     *
     * @param Text the new text-title of the window
     */
    virtual void SetWindowTitle ( const AnsiChar *const Text );
};

} // namespace LumenEngine
