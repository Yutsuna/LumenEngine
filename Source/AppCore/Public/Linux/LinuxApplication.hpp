/**
 * @file LinuxApplication.hpp
 * @brief Declaration of the FLinuxApplication class for Linux-specific application management.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_LINUX )

    #include <AppCoreExport.hpp>

    #include <Generic/GenericApplication.hpp>
    #include <Linux/LinuxBackend.hpp>

    #include <Templates/Vector.hpp>

    #include <SDL3/SDL_events.h>

namespace Lumen
{

class FLinuxWindow;

class APPCORE_EXPORT FLinuxApplication : public FGenericApplication
{
public:

    FLinuxApplication () noexcept = default;
    ~FLinuxApplication () noexcept override;

public:

    /** @brief Polls SDL Messages */
    void PumpMessages ( Float64 DeltaTime ) noexcept override;

    /** @brief Creates a new Linux window */
    [[nodiscard]] TSharedRef<FGenericWindow> MakeWindow () override;

    /** @brief Initialize a Linux window with the given parameters */
    void InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                            const TSharedRef<FGenericWindowDescription> &InDescription,
                            const TSharedPtr<FGenericWindow> &InParentWindow,
                            Bool bShowImmediately ) noexcept override;

    void AddPendingEvent ( const SDL_Event &InEvent ) noexcept;

    /** @brief Shuts down the application. */
    void Shutdown () noexcept override;

public:

    /** @brief Factory to create the Application instance */
    [[nodiscard]] static TSharedPtr<FGenericApplication> MakeLinuxApplication () noexcept;

private:

    /** @brief helper function to find a window by its SDL Window ID */
    [[nodiscard]] TSharedPtr<FLinuxWindow> FindWindowByID ( SDL_WindowID InWindowID ) const noexcept;

private:

    TVector<TSharedRef<FLinuxWindow>> Windows;
};

extern APPCORE_EXPORT FLinuxApplication *GLinuxApplication;

} // namespace Lumen

#endif
