/**
 * @file GenericApplication.cpp
 * @brief Implementation of the FGenericApplication class for cross-platform application management.
 */

#include "Generic/GenericApplication.hpp"
#include "Generic/GenericApplicationMessageHandler.hpp"
#include "Generic/GenericWindow.hpp"

/**
 * public
 */

LumenEngine::FGenericApplication::FGenericApplication () : MessageHandler( MakeShared<FGenericApplicationMessageHandler>() ), Cursor( nullptr )
{
    /* Ctor */
}

void LumenEngine::FGenericApplication::SetMessageHandler ( const TSharedRef<FGenericApplicationMessageHandler> &InMessageHandler )
{
    MessageHandler = InMessageHandler;
}

LumenEngine::TSharedPtr<LumenEngine::FGenericApplicationMessageHandler> LumenEngine::FGenericApplication::GetMessageHandler () const
{
    return MessageHandler;
}

void LumenEngine::FGenericApplication::PumpMessages ( const Float32 DeltaTime )
{
    /* Empty */
}

LumenEngine::TSharedRef<LumenEngine::FGenericWindow> LumenEngine::FGenericApplication::MakeWindow ()
{
    return MakeShareable( new FGenericWindow() );
}

void LumenEngine::FGenericApplication::InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                                                          const TSharedRef<FGenericWindowDescription> &InDescription,
                                                          const TSharedPtr<FGenericWindow> &InParentWindow,
                                                          const Bool bShowImmediately )
{
    /* Empty */
}
