///
/// @file GenericApplication.cpp
/// @brief Implementation of the FGenericApplication class for cross-platform application management.
///

#include <Generic/GenericApplication.hpp>
#include <Generic/GenericApplicationMessageHandler.hpp>
#include <Generic/GenericWindow.hpp>

///
/// public
///

Lumen::FGenericApplication::FGenericApplication () : MessageHandler( MakeShared<FGenericApplicationMessageHandler>() )
{
    /// Ctor
}

void Lumen::FGenericApplication::SetMessageHandler ( const TSharedRef<FGenericApplicationMessageHandler> &InMessageHandler )
{
    MessageHandler = InMessageHandler;
}

Lumen::TSharedPtr<Lumen::FGenericApplicationMessageHandler> Lumen::FGenericApplication::GetMessageHandler () const
{
    return MessageHandler;
}

void Lumen::FGenericApplication::PumpMessages ( const Float64 __attribute__( ( unused ) ) DeltaTime )
{
    /// Empty
}

Lumen::TSharedRef<Lumen::FGenericWindow> Lumen::FGenericApplication::MakeWindow ()
{
    return MakeShareable( new FGenericWindow() );
}

void Lumen::FGenericApplication::InitializeWindow ( const TSharedRef<FGenericWindow> __attribute__( ( unused ) ) & InWindow,
                                                    const TSharedRef<FGenericWindowDescription> __attribute__( ( unused ) ) &
                                                        InDescription,
                                                    const TSharedPtr<FGenericWindow> __attribute__( ( unused ) ) & InParentWindow,
                                                    const Bool __attribute__( ( unused ) ) bShowImmediately )

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

void Lumen::FGenericApplication::Shutdown () noexcept
{
    /// Empty
}
