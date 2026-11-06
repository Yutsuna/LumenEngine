/**
- @file GenericApplication.cpp
- @brief Implementation of the FGenericApplication class for cross-platform application management.
 */

#include <Generic/GenericApplication.hpp>
#include <Generic/GenericApplicationMessageHandler.hpp>
#include <Generic/GenericWindow.hpp>

/**
- public
 */

Lumen::FGenericApplication::FGenericApplication () noexcept : MessageHandler( MakeShared<FGenericApplicationMessageHandler>() )
{
    /** Ctor */
}

void Lumen::FGenericApplication::SetMessageHandler (
    const TSharedRef<FGenericApplicationMessageHandler> &InMessageHandler ) noexcept
{
    MessageHandler = InMessageHandler;
}

Lumen::TSharedPtr<Lumen::FGenericApplicationMessageHandler> Lumen::FGenericApplication::GetMessageHandler () const noexcept
{
    return MessageHandler;
}

Lumen::TSharedRef<Lumen::FGenericWindow> Lumen::FGenericApplication::MakeWindow ()
{
    return MakeShareable( new FGenericWindow() );
}

void Lumen::FGenericApplication::InitializeWindow ( const TSharedRef<FGenericWindow> LUMEN_UNUSED &InWindow,
                                                    const TSharedRef<FGenericWindowDescription> LUMEN_UNUSED &InDescription,
                                                    const TSharedPtr<FGenericWindow> LUMEN_UNUSED &InParentWindow,
                                                    const Bool LUMEN_UNUSED bShowImmediately ) noexcept

{
    if ( not MainWindow.IsValid() )
    {
        MainWindow = InWindow;
    }
}

Lumen::TSharedPtr<Lumen::FGenericWindow> Lumen::FGenericApplication::GetMainWindow () const noexcept
{
    return MainWindow;
}

void Lumen::FGenericApplication::PumpMessages ( const Float64 LUMEN_UNUSED DeltaTime ) noexcept
{
    /** Empty */
}

void Lumen::FGenericApplication::Shutdown () noexcept
{
    /** Empty */
}
