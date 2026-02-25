/**
 * @file GenericApplicationMessageHandler.cpp
 * @brief Default Implementation of the FGenericApplicationMessageHandler class for handling application messages.
 */

#include "Generic/GenericApplicationMessageHandler.hpp"

/** Application */

void LumenEngine::FGenericApplicationMessageHandler::OnRequestExit ()
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnQuit ()
{
    /* Empty */
}

/** Window */

void LumenEngine::FGenericApplicationMessageHandler::OnOSPaint ( const TSharedRef<FGenericWindow> &InWindow )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnWindowMoved ( const TSharedRef<FGenericWindow> &InWindow, const Int32 InX, const Int32 InY )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnWindowResized ( const TSharedRef<FGenericWindow> &InWindow, const Int32 InWidth, const Int32 InHeight )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnWindowActivationChanged ( const TSharedRef<FGenericWindow> &InWindow, const Bool bIsActive )
{
    /* Empty */
}

/** Mouse */

void LumenEngine::FGenericApplicationMessageHandler::OnMouseDown ( const EMouseButton::Type InButton )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnMouseUp ( const EMouseButton::Type InButton )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnMouseMove ( const Int32 InX, const Int32 InY )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnMouseWheel ( const Int32 InDelta )
{
    /* Empty */
}

/** Keyboard */

void LumenEngine::FGenericApplicationMessageHandler::OnKeyDown ( const EKeys::Type InKey, const Bool bIsRepeat )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnKeyUp ( const EKeys::Type InKey )
{
    /* Empty */
}
