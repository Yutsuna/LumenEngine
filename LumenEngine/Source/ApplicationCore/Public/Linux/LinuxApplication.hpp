/**
 * @file LinuxApplication.hpp
 * @brief Declaration of the FLinuxApplication class for Linux-specific application management.
 */

#pragma once

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )

    #include "Definitions.hpp"

    #include "Container/Vector.hpp"
    #include "Generic/GenericApplication.hpp"

    #include <SDL3/SDL.h>

namespace LumenEngine
{

class FLinuxWindow;

class LUMEN_ENGINE_API FLinuxApplication : public FGenericApplication
{
public:

    FLinuxApplication () noexcept = default;
    ~FLinuxApplication () override;

    /** Polls SDL messages */
    void PumpMessages ( const Float64 DeltaTime ) override;

    /** Creates a new Linux window */
    TSharedRef<FGenericWindow> MakeWindow () override;

    /** Initializes a Linux window with the given parameters */
    void InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                            const TSharedRef<FGenericWindowDescription> &InDescription,
                            const TSharedPtr<FGenericWindow> &InParentWindow,
                            const Bool bShowImmediately ) override;

    /** Adds a pending SDL event to be processed in the next message pump */
    void AddPendingEvent ( const SDL_Event &InEvent );

public:

    /** Factory to create the application instance */
    static TSharedPtr<FGenericApplication> CreateLinuxApplication ();

private:

    /** Helper function to find a window by its SDL Window ID */
    [[nodiscard]] TSharedPtr<FLinuxWindow> FindWindowByID ( const SDL_WindowID InWindowID ) const;

private:

    TVector<TSharedRef<FLinuxWindow>> Windows;
};

extern FLinuxApplication *GLinuxApplication;

} // namespace LumenEngine

#endif