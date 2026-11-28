/**
 * @file WindowsApplication.hpp
 * @brief Declaration of the FWindowsApplication class for Windows-specific application management.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_WINDOWS )

    #include <AppCoreExport.hpp>
    #include <Core/CoreTypes.hpp>

    #include <Generic/GenericApplication.hpp>

    #include <Templates/Optional.hpp>
    #include <Templates/Vector.hpp>

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>

namespace Lumen
{

class FWindowsWindow;

/**
 * @class FWindowsApplication
 * @brief Windows (Win32-backed) implementation of the application interface.
 */
class APPCORE_EXPORT FWindowsApplication : public FGenericApplication
{
public:

    FWindowsApplication () noexcept;
    ~FWindowsApplication () noexcept override;

public:

    /** @brief Polls and dispatches Win32 messages. */
    void PumpMessages ( Float64 DeltaTime ) noexcept override;

    /** @brief Creates a new Windows window. */
    [[nodiscard]] TSharedRef<FGenericWindow> MakeWindow () noexcept override;

    /** @brief Initializes a Windows window with the given parameters. */
    void InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                            const TSharedRef<FGenericWindowDescription> &InDescription,
                            const TSharedPtr<FGenericWindow> &InParentWindow,
                            Bool bShowImmediately ) noexcept override;

    /** @brief Shuts down the application and unregisters the Win32 window class. */
    void Shutdown () noexcept override;

public:

    /** @brief Factory to create the Application instance. */
    [[nodiscard]] static TSharedPtr<FGenericApplication> MakeWindowsApplication () noexcept;

    /** @return The Win32 module instance handle owning the application windows. */
    [[nodiscard]] HINSTANCE GetOSInstanceHandle () const noexcept;

    /** @brief Processes a single Win32 window message and dispatches it to the message handler. */
    [[nodiscard]] LRESULT ProcessMessage ( HWND InWindowHandle, UINT InMessage, WPARAM InWParam, LPARAM InLParam ) noexcept;

private:

    /** @brief Win32 window procedure callback registered on the application window class. */
    static LRESULT CALLBACK AppWndProc ( HWND InWindowHandle, UINT InMessage, WPARAM InWParam, LPARAM InLParam ) noexcept;

    /** @brief Ensures the Win32 window class is registered for the current HINSTANCE. */
    Bool EnsureWindowClassRegistered () noexcept;

    /** @brief Helper function to find a window by its Win32 HWND. */
    [[nodiscard]] TOptional<TSharedRef<FWindowsWindow>> FindWindowByHandle ( HWND InWindowHandle ) const noexcept;

private:

    TVector<TSharedRef<FWindowsWindow>> Windows;
    HINSTANCE InstanceHandle    = nullptr;
    Bool bWindowClassRegistered = false;
};

extern APPCORE_EXPORT TAtomic<FWindowsApplication *> GWindowsApplication;

} // namespace Lumen

#endif
