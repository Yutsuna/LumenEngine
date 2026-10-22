/// @file GenericApplication.hpp
/// @brief Base class for platform-independent application management.

#pragma once

#include <AppCoreExport.hpp>

#include <Memory/SharedPtr.hpp>

namespace Lumen
{

class FGenericWindow;
struct FGenericWindowDescription;
class FGenericApplicationMessageHandler;

class APPCORE_EXPORT FGenericApplication
{

public:

    FGenericApplication () noexcept;
    virtual ~FGenericApplication () noexcept = default;

public:

    /// @brief Sets the message handler for the application.
    void SetMessageHandler ( const TSharedRef<FGenericApplicationMessageHandler> &MessageHandler ) noexcept;

    /// @return The message handler for the application.
    [[nodiscard]] TSharedRef<FGenericApplicationMessageHandler> &GetMessageHandler () const noexcept;

    /// @brief Initializes a window for the application.
    virtual void InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                                    const TSharedRef<FGenericWindowDescription> &InDescription,
                                    const TSharedRef<FGenericWindow> &InParentWindow,
                                    Bool bShowImmediately ) noexcept;

    /// @return The main window for the application.
    [[nodiscard]] virtual TSharedPtr<FGenericWindow> GetMainWindow () const noexcept;

    /// @brief Shuts down the application.
    virtual void Shutdown () noexcept;

protected:

    TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;
    TSharedPtr<FGenericWindow> MainWindow;
};

extern APPCORE_EXPORT TSharedPtr<FGenericApplication> GPlatformApplication;

} // namespace Lumen