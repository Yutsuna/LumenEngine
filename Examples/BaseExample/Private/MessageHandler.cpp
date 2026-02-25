/**
 * @file MessageHandler.cpp
 * @brief Message handler for the base example application implementation.
 */

#include "MessageHandler.hpp"

#include "LaunchEngineLoop.hpp"

#include <iostream>

void LumenEngine::FBaseExampleMessageHandler::OnWindowCloseRequested ( const TSharedRef<FGenericWindow> &InWindow )
{
    std::cout << "HELLOOOO" << std::endl;
    GEngineLoop.RequestExit( "Event: Window close requested" );
}

void LumenEngine::FBaseExampleMessageHandler::OnRequestExit ()
{
    std::cout << "Application exit requested." << std::endl;
    GEngineLoop.RequestExit( "Event: Application exit requested" );
}
