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

    /** Window message handler */

    /** Called when a window needs to be repainted */
    virtual Bool OnOSPaint ( const TSharedRef<FGenericWindow> &InWindow );

    /** Called when a window is moved */
    virtual Bool OnWindowMoved ( const TSharedRef<FGenericWindow> &InWindow, const Int32 InX, const Int32 InY );

    /** Called when a window is resized */
    virtual Bool OnWindowResized ( const TSharedRef<FGenericWindow> &InWindow, const Int32 InWidth, const Int32 InHeight );

    /** Called when a window activation state changes */
    virtual Bool OnWindowActivationChanged ( const TSharedRef<FGenericWindow> &InWindow, const Bool bIsActive );

    /** Called when a window is requested to close */
    virtual Bool OnWindowCloseRequested ( const TSharedRef<FGenericWindow> &InWindow );

public:

    /** Mouse message handler */

    /** Called when a mouse button is pressed */
    virtual Bool OnMouseDown ( const TSharedPtr<FGenericWindow> &InWindow, const Int32 InButton );

    /** Called when a mouse button is released */
    virtual Bool OnMouseUp ( const Int32 InButton );

    /** Called when the mouse is moved */
    virtual Bool OnMouseMove ( const Int32 InX, const Int32 InY );

    /** Called when the mouse wheel is scrolled */
    virtual Bool OnMouseWheel ( const Int32 InDelta );

public:

    /** Keyboard message handler */

    /** Called when a key is pressed */
    virtual Bool OnKeyDown ( const EKeys::Type Inkey, const Bool bIsRepeat );

    /** Called when a key is released */
    virtual Bool OnKeyUp ( const EKeys::Type InKey, const Bool bIsRepeat );
};

} // namespace LumenEngine
