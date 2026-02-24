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
class FGenericCursor;

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
    TSharedPtr<FGenericApplicationMessageHandler> GetMessageHandler () const;

    /** Polls and processes messages for the application */
    virtual void PumpMessages ( const Float32 DeltaTime );

    /** Sets the cursor for the application */
    virtual void SetCursor ( const TSharedPtr<FGenericCursor> &InCursor );

    /** Retrieves the current cursor of the application */
    TSharedPtr<FGenericCursor> GetCursor () const;

    /** Creates a new window */
    virtual TSharedRef<FGenericWindow> MakeWindow ();

    /** Initializes a window with the given parameters */
    virtual void InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                                    const TSharedRef<FGenericWindowDescription> &InDescription,
                                    const TSharedPtr<FGenericWindow> &InParentWindow,
                                    const bool bShowImmediately );

protected:

    TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;
    TSharedPtr<FGenericCursor> Cursor;
};

} // namespace LumenEngine
