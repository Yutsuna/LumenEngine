/**
 * @file MessageHandler.cpp
 * @brief Implements the FLuminaMessageHandler class.
 */

#include <MessageHandler.hpp>

#include <LogLumina.hpp>

#include <utility>

/**
 * Public
 */

Lumen::FLuminaMessageHandler::FLuminaMessageHandler ( TSharedPtr<FGenericApplicationMessageHandler> InNextHandler ) noexcept
    : NextHandler( std::move( InNextHandler ) )
{
    /** Empty */
}

void Lumen::FLuminaMessageHandler::OnRequestExit () noexcept
{
    LUMEN_LOG_VERY_VERBOSE( LogLumina, "Exit requested by the platform" );

    if ( NextHandler.IsValid() ) [[likely]]
    {
        NextHandler->OnRequestExit();
    }
}

void Lumen::FLuminaMessageHandler::OnQuit () noexcept
{
    LUMEN_LOG_VERY_VERBOSE( LogLumina, "Quit requested by the platform" );

    if ( NextHandler.IsValid() ) [[likely]]
    {
        NextHandler->OnQuit();
    }
}
