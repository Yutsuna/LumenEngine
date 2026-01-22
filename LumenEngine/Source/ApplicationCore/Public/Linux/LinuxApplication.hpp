/**
 * @file LinuxApplication.hpp
 * @brief Declaration of the FLinuxApplication class for Linux-specific application management.
 */

#pragma once

#include "Container/Vector.hpp"
#include "Generic/GenericApplication.hpp"

namespace LumenEngine
{

class FLinuxWindow;

class FLinuxApplication : public FGenericApplication
{
public:

    FLinuxApplication ();
    ~FLinuxApplication () override;

    /** Polls SDL messages */
    void PumpMessages ( const Float32 DeltaTime ) override;

    /** Creates a new Linux window */
    TSharedRef<FGenericWindow> MakeWindow () override;

    /** Initializes a Linux window with the given parameters */
    void InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow, const TSharedRef<FGenericWindowDescription> &InDescription, const TSharedPtr<FGenericWindow> &InParentWindow, const bool bShowImmediately ) override;

    /** Factory to create the application instance */
    static TSharedRef<FGenericApplication> CreateLinuxApplication ();

private:

    FVector<TSharedRef<FLinuxWindow>> Windows;
};

} // namespace LumenEngine
