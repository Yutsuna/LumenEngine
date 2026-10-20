/// @file GenericApplicationMessageHandler.hpp
/// @brief Declaration of the FGenericApplicationMessageHandler class for handling application messages.

#pragma once

#include <AppCoreExport.hpp>

#include <Generic/GenericApplicationInput.hpp>
#include <Memory/SharedPtr.hpp>

namespace Lumen
{

class FGenericWindow;

class APPCORE_EXPORT FGenericApplicationMessageHandler
{

public:

    virtual ~FGenericApplicationMessageHandler () noexcept = default;

public:

    /// Application message handler

    /// @brief Called when the application is requested to exit
    virtual void OnRequestExit () noexcept;

    /// @brief Called when the application is quitting
    virtual void OnQuit () noexcept;

public:

    /// Window message

    /// @brief Called when a window needs to be repainted
    virtual void OnOSPaint ( const TSharedRef<FGenericWindow> &InWindow ) noexcept;

    /// @brief Called when a window is moved
    virtual void OnWindowMoved ( const TSharedRef<FGenericWindow> &InWindow ) noexcept;

    /// @brief Called when a window is resized
    virtual void OnWindowResized ( const TSharedRef<FGenericWindow> &InWindow ) noexcept;

    /// @brief Called when a window activation state changes
    virtual void OnWindowActivationChanged ( const TSharedRef<FGenericWindow> &InWindow ) noexcept;

public:

    /// Mouse message handler

    /// @brief Called when a mouse button is pressed
    virtual void OnMouseDown ( EMouseButton::Type InMouseButton ) noexcept;

    /// @brief Called when a mouse button is released
    virtual void OnMouseUp ( EMouseButton::Type InMouseButton ) noexcept;

    /// @brief Called when a mouse is moved
    virtual void OnMouseMove ( Int32 InX, Int32 InY ) noexcept;

    /// @brief Called when the mouse wheel is scrolled
    virtual void OnMouseWheel ( Int32 InDelta ) noexcept;

public:

    /// Keyboard message handler

    /// @brief Called when a key is pressed
    virtual void OnKeyDown ( EKeys::Type InKey, Bool bIsRepeated ) noexcept;

    /// @brief Called when a key is released
    virtual void OnKeyUp ( EKeys::Type InKey, Bool bIsRepeated ) noexcept;
};

} // namespace Lumen