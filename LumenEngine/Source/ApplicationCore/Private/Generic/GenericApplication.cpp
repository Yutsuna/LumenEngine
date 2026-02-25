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
    /* __ctor__ */
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
    /* __empty__ */
}

void LumenEngine::FGenericApplication::SetCursor ( const TSharedPtr<FGenericCursor> &InCursor )
{
    Cursor = InCursor;
}

LumenEngine::TSharedPtr<LumenEngine::FGenericCursor> LumenEngine::FGenericApplication::GetCursor () const
{
    return Cursor;
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
    /* __empty__ */
}
