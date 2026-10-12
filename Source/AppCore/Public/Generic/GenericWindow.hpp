/// AppCore/Generic/GenericWindow.hpp
/// Declaration of the FGenericWindow class for cross-platform window management.

#pragma once

#include <AppCoreExport.hpp>

// TODO:
// #include <Core/Container/Memory/SharedPtr.hpp>

namespace Lumen
{

class APPCORE_EXPORT FGenericWindow
{

public:

    FGenericWindow ()          = default;
    virtual ~FGenericWindow () = default;

    /// @brief Clear the window's content
    virtual void Clear ();

    /// @brief Ask the OS to destroy OS-specific resource associated with the window
    virtual void Destroy ();

    /// @brief Shows the OS window on the screen
    virtual void Show ();

protected:

    // TSharedPtr<FGenericWindowDescription> Description;
};

} // namespace Lumen