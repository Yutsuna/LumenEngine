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

void LumenEngine::FGenericApplicationMessageHandler::OnOSPaint ( const TSharedRef<FGenericWindow> & )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnWindowMoved ( const TSharedRef<FGenericWindow> &, const Int32, const Int32 )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnWindowResized ( const TSharedRef<FGenericWindow> &, const Int32, const Int32 )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnWindowActivationChanged ( const TSharedRef<FGenericWindow> &, const Bool )
{
    /* Empty */
}

/** Mouse */

void LumenEngine::FGenericApplicationMessageHandler::OnMouseDown ( const EMouseButton::Type )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnMouseUp ( const EMouseButton::Type )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnMouseMove ( const Int32, const Int32 )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnMouseWheel ( const Int32 )
{
    /* Empty */
}

/** Keyboard */

void LumenEngine::FGenericApplicationMessageHandler::OnKeyDown ( const EKeys::Type, const Bool )
{
    /* Empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnKeyUp ( const EKeys::Type )
{
    /* Empty */
}
