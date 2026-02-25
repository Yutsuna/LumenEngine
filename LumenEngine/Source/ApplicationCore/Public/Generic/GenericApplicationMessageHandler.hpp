/**
 * @file GenericApplicationMessageHandler.hpp
 * @brief Declaration of the FGenericApplicationMessageHandler class for handling application messages.
 */

#pragma once

#include "Container/SharedPtr.hpp"
#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "GenericApplicationInput.hpp"

namespace LumenEngine
{

class FGenericWindow;

/**
 * @class FGenericApplicationMessageHandler
 * @brief Interface for handling platform-specific application messages.
 */
class LUMEN_ENGINE_API FGenericApplicationMessageHandler
{
public:

    virtual ~FGenericApplicationMessageHandler () = default;

public:

    /** Application message handler */

    /** Called when the application is requested to exit */
    virtual void OnRequestExit ();

    /** Called when the application is quitting */
    virtual void OnQuit ();

public:

    /** Window message handler */

    /** Called when a window needs to be repainted */
    virtual void OnOSPaint ( const TSharedRef<FGenericWindow> &InWindow );

    /** Called when a window is moved */
    virtual void OnWindowMoved ( const TSharedRef<FGenericWindow> &InWindow, const Int32 InX, const Int32 InY );

    /** Called when a window is resized */
    virtual void OnWindowResized ( const TSharedRef<FGenericWindow> &InWindow, const Int32 InWidth, const Int32 InHeight );

    /** Called when a window activation state changes */
    virtual void OnWindowActivationChanged ( const TSharedRef<FGenericWindow> &InWindow, const Bool bIsActive );

public:

    /** Mouse message handler */

    /** Called when a mouse button is pressed */
    virtual void OnMouseDown ( const EMouseButton::Type InButton );

    /** Called when a mouse button is released */
    virtual void OnMouseUp ( const EMouseButton::Type InButton );

    /** Called when the mouse is moved */
    virtual void OnMouseMove ( const Int32 InX, const Int32 InY );

    /** Called when the mouse wheel is scrolled */
    virtual void OnMouseWheel ( const Int32 InDelta );

public:

    /** Keyboard message handler */

    /** Called when a key is pressed */
    virtual void OnKeyDown ( const EKeys::Type Inkey, const Bool bIsRepeat );

    /** Called when a key is released */
    virtual void OnKeyUp ( const EKeys::Type InKey );
};

} // namespace LumenEngine
