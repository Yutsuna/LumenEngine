/**
 * @file MessageHandler.cpp
 * @brief Message handler for the base example application implementation.
 */

#include "MessageHandler.hpp"

#include "LaunchEngineLoop.hpp"

void LumenEngine::FBaseExampleMessageHandler::OnWindowCloseRequested ( const TSharedRef<FGenericWindow> &InWindow )
{
    GEngineLoop.RequestExit( "Event: Window close requested" );
}

void LumenEngine::FBaseExampleMessageHandler::OnRequestExit ()
{
    GEngineLoop.RequestExit( "Event: Application exit requested" );
}
