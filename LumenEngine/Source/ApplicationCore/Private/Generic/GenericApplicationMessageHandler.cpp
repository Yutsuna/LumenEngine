/**
 * @file GenericApplicationMessageHandler.cpp
 * @brief Default Implementation of the FGenericApplicationMessageHandler class for handling application messages.
 */

#include "Generic/GenericApplicationMessageHandler.hpp"

/** Window */

void LumenEngine::FGenericApplicationMessageHandler::OnOSPaint ( const TSharedRef<FGenericWindow> &InWindow )
{
    /* empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnWindowMoved ( const TSharedRef<FGenericWindow> &InWindow, const Int32 InX, const Int32 InY )
{
    /* empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnWindowResized ( const TSharedRef<FGenericWindow> &InWindow, const Int32 InWidth, const Int32 InHeight )
{
    /* empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnWindowActivationChanged ( const TSharedRef<FGenericWindow> &InWindow, const Bool bIsActive )
{
    /* empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnWindowCloseRequested ( const TSharedRef<FGenericWindow> &InWindow )
{
    /* empty */
}

/** Mouse */

void LumenEngine::FGenericApplicationMessageHandler::OnMouseDown ( const TSharedPtr<FGenericWindow> &InWindow, const Int32 InButton )
{
    /* empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnMouseUp ( const Int32 InButton )
{
    /* empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnMouseMove ( const Int32 InX, const Int32 InY )
{
    /* empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnMouseWheel ( const Int32 InDelta )
{
    /* empty */
}

/** Keyboard */

void LumenEngine::FGenericApplicationMessageHandler::OnKeyDown ( const EKeys::Type InKey )
{
    /* empty */
}

void LumenEngine::FGenericApplicationMessageHandler::OnKeyUp ( const EKeys::Type InKey )
{
    /* empty */
}
