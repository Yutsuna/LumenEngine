/**
 * @file LinuxApplication.hpp
 * @brief Declaration of the FLinuxApplication class for Linux-specific application management.
 */

#pragma once

#include "Container/Vector.hpp"
#include "Definitions.hpp"
#include "Generic/GenericApplication.hpp"

#include <SDL3/SDL.h>

namespace LumenEngine
{

class FLinuxWindow;

class LUMEN_ENGINE_API FLinuxApplication : public FGenericApplication
{
public:

    FLinuxApplication ();
    ~FLinuxApplication () override;

    /** Polls SDL messages */
    void PumpMessages ( const Float32 DeltaTime ) override;

    /** Creates a new Linux window */
    TSharedRef<FGenericWindow> MakeWindow () override;

    /** Initializes a Linux window with the given parameters */
    void InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                            const TSharedRef<FGenericWindowDescription> &InDescription,
                            const TSharedPtr<FGenericWindow> &InParentWindow,
                            const bool bShowImmediately ) override;

    /** Adds a pending SDL event to be processed in the next message pump */
    void AddPendingEvent ( const SDL_Event &InEvent );

public:

    /** Factory to create the application instance */
    static TSharedPtr<FGenericApplication> CreateLinuxApplication ();

private:

    /** Helper function to find a window by its SDL Window ID */
    TSharedPtr<FLinuxWindow> FindWindowByID ( const SDL_WindowID InWindowID ) const;

private:

    FVector<TSharedRef<FLinuxWindow>> Windows;
};

extern FLinuxApplication *GLinuxApplication;

} // namespace LumenEngine
