/**
 * @file MessageHandler.cpp
 * @brief Message handler for the base example application implementation.
 */

#include "MessageHandler.hpp"
#include "LaunchEngineLoop.hpp"

namespace LumenEngine
{

void FTriangleExampleMessageHandler::OnRequestExit ()
{
    GEngineLoop.RequestExit( "Event: Application exit requested" );
}

} // namespace LumenEngine
