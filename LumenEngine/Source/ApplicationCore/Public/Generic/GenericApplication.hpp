/**
 * @file GenericApplication.hpp
 * @brief Declaration of the FGenericApplication class for cross-platform application management.
 */

#pragma once

#include "Container/SharedPtr.hpp"
#include "Definitions.hpp"

namespace LumenEngine
{

/**
 * Forwards
 */

class FGenericWindow;
struct FGenericWindowDescription;
class FGenericApplicationMessageHandler;

/**
 * @class FGenericApplication
 * @brief A base class for platform-independent application management.
 * @details Provides a common interface for handling application-level functionality
 */
class LUMEN_ENGINE_API FGenericApplication
{
public:

    FGenericApplication ();
    virtual ~FGenericApplication () = default;

    /** Sets the message handler for the application */
    virtual void SetMessageHandler ( const TSharedRef<FGenericApplicationMessageHandler> &InMessageHandler );

    /** Retrieves the current message handler of the application */
    [[nodiscard]] TSharedPtr<FGenericApplicationMessageHandler> GetMessageHandler () const;

    /** Polls and processes messages for the application */
    virtual void PumpMessages ( const Float64 DeltaTime );

    /** Creates a new window */
    [[nodiscard]] virtual TSharedRef<FGenericWindow> MakeWindow ();

    /** Initializes a window with the given parameters */
    virtual void InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                                    const TSharedRef<FGenericWindowDescription> &InDescription,
                                    const TSharedPtr<FGenericWindow> &InParentWindow,
                                    const Bool bShowImmediately );

    [[nodiscard]] virtual TSharedPtr<FGenericWindow> GetMainWindow () const noexcept;

protected:

    TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;
    TSharedPtr<FGenericWindow> MainWindow;
};

extern LUMEN_ENGINE_API TSharedPtr<FGenericApplication> GPlatformApplication;

} // namespace LumenEngine
