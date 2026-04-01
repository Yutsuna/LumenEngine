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

LumenEngine::FGenericApplication::FGenericApplication () : MessageHandler( MakeShared<FGenericApplicationMessageHandler>() )
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

void LumenEngine::FGenericApplication::PumpMessages ( const Float64 __attribute__( ( unused ) ) DeltaTime )
{
    /* Empty */
}

LumenEngine::TSharedRef<LumenEngine::FGenericWindow> LumenEngine::FGenericApplication::MakeWindow ()
{
    return MakeShareable( new FGenericWindow() );
}

void LumenEngine::FGenericApplication::InitializeWindow ( const TSharedRef<FGenericWindow> __attribute__( ( unused ) ) & InWindow,
                                                          const TSharedRef<FGenericWindowDescription> __attribute__( ( unused ) ) & InDescription,
                                                          const TSharedPtr<FGenericWindow> __attribute__( ( unused ) ) & InParentWindow,
                                                          const Bool __attribute__( ( unused ) ) bShowImmediately )

{
    if ( not MainWindow.IsValid() )
    {
        MainWindow = InWindow;
    }
}

LumenEngine::TSharedPtr<LumenEngine::FGenericWindow> LumenEngine::FGenericApplication::GetMainWindow () const noexcept
{
    return MainWindow;
}
