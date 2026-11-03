///
/// @file LinuxApplication.cpp
/// @brief Implementation of the FLinuxApplication class for Linux-specific application management.
///

#include <Linux/LinuxApplication.hpp>

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )

    #include <Linux/LinuxBackend.hpp>
    #include <Linux/LinuxWindow.hpp>

    #include "Container/Signal.hpp"

    #include <Generic/GenericApplicationInput.hpp>
    #include <Generic/GenericApplicationMessageHandler.hpp>

    #include <Logging/Logger.hpp>
    #include <Logging/LoggingCategory.hpp>

    #include <SDL3/SDL_events.h>
    #include <SDL3/SDL.h>

Lumen::FLinuxApplication *Lumen::GLinuxApplication = nullptr;

namespace
{

const Lumen::FLogCategory LogLinuxApplication( "LinuxApplication" );

}

Lumen::FLinuxApplication::~FLinuxApplication ()
{
    Shutdown();

    if ( GLinuxApplication == this )
    {
        GLinuxApplication = nullptr;
    }
}

Lumen::TSharedRef<Lumen::FGenericWindow> Lumen::FLinuxApplication::MakeWindow ()
{
    return FLinuxWindow::Make();
}

Lumen::TSharedPtr<Lumen::FGenericApplication> Lumen::FLinuxApplication::CreateLinuxApplication ()
{
    if ( not FLinuxBackend::InitializeSDL() )
    {
        LUMEN_LOG_FATAL( LogLinuxApplication, "Failed to initialize SDL library for Linux application: {}", SDL_GetError() );
        /// INFO: unused as LUMEN LOG FATAL with raise a terminate signal after logging the message
        return nullptr;
    }

    /// Use MakeShared to align the control block and pointer cleanly together
    TSharedPtr<FLinuxApplication> Application = MakeShared<FLinuxApplication>();
    GLinuxApplication                         = Application.Get();

    return StaticCastSharedPtr<FGenericApplication>( Application );
}

void Lumen::FLinuxApplication::InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                                                        const TSharedRef<FGenericWindowDescription> &InDescription,
                                                        const TSharedPtr<FGenericWindow> &InParentWindow,
                                                        const Bool bShowImmediately )
{
    const TSharedRef<FLinuxWindow> LinuxWindow  = StaticCastSharedRef<FLinuxWindow>( InWindow );
    const TSharedPtr<FLinuxWindow> ParentWindow = StaticCastSharedPtr<FLinuxWindow>( InParentWindow );

    LinuxWindow->Initialize( this, InDescription, InParentWindow, bShowImmediately );
    Windows.push_back( LinuxWindow );
}

void Lumen::FLinuxApplication::Shutdown () noexcept
{
    /// INFO: Clear all tracked windows and message handlers
    Windows.clear();
    MainWindow.Reset();
    MessageHandler.Reset();
    FLinuxBackend::ShutdownSDL();
}

namespace Lumen
{

namespace
{

    inline void SendKeyDownEvent ( const TSharedPtr<FGenericApplicationMessageHandler> &MessageHandler, const SDL_KeyboardEvent &KeyboardEvent )
    {
        const SDL_Keycode KeyCode = KeyboardEvent.key;
        const EKeys::Type KeyType = TranslateSDLKeyCodeToEKeys( KeyCode );
        const Bool bIsRepeated    = KeyboardEvent.repeat != 0;

        MessageHandler->OnKeyDown( KeyType, bIsRepeated );
    }

    inline void SendKeyUpEvent ( const TSharedPtr<FGenericApplicationMessageHandler> &MessageHandler, const SDL_KeyboardEvent &KeyboardEvent )
    {
        const SDL_Keycode KeyCode = KeyboardEvent.key;
        const EKeys::Type KeyType = TranslateSDLKeyCodeToEKeys( KeyCode );

        MessageHandler->OnKeyUp( KeyType );
    }

    inline void SendMouseButtonDownEvent ( const TSharedPtr<FGenericApplicationMessageHandler> &MessageHandler, const SDL_MouseButtonEvent &MouseButtonEvent )
    {
        const EMouseButton::Type ButtonType = TranslateSDLMouseButtonToEMouseButton( MouseButtonEvent.button );

        MessageHandler->OnMouseDown( ButtonType );
    }

    inline void SendMouseButtonUpEvent ( const TSharedPtr<FGenericApplicationMessageHandler> &MessageHandler, const SDL_MouseButtonEvent &MouseButtonEvent )
    {
        const EMouseButton::Type ButtonType = TranslateSDLMouseButtonToEMouseButton( MouseButtonEvent.button );

        MessageHandler->OnMouseUp( ButtonType );
    }

    inline void SendQuitEvent ( const TSharedPtr<FGenericApplicationMessageHandler> &MessageHandler )
    {
        MessageHandler->OnQuit();
        FSignal::Raise( ESystemSignal::Terminate );
    }

    inline void SendWindowCloseRequestedEvent ( const TSharedPtr<FGenericApplicationMessageHandler> &MessageHandler )
    {
        MessageHandler->OnRequestExit();
    }

} // namespace

} // namespace Lumen

void Lumen::FLinuxApplication::AddPendingEvent ( const SDL_Event &InEvent )
{
    TSharedPtr<FLinuxWindow> CurrentWindow = FindWindowByID( InEvent.window.windowID );

    if ( not CurrentWindow.IsValid() )
    {
        return;
    }

    switch ( InEvent.type )
    {
    case SDL_EVENT_QUIT:
    {
        SendQuitEvent( MessageHandler );
        break;
    }
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    {
        SendWindowCloseRequestedEvent( MessageHandler );
        break;
    }
    case SDL_EVENT_KEY_DOWN:
    {
        SendKeyDownEvent( MessageHandler, InEvent.key );
        break;
    }
    case SDL_EVENT_KEY_UP:
    {
        SendKeyUpEvent( MessageHandler, InEvent.key );
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        SendMouseButtonDownEvent( MessageHandler, InEvent.button );
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        SendMouseButtonUpEvent( MessageHandler, InEvent.button );
        break;
    }
    default:
        break;
    }
}

void Lumen::FLinuxApplication::PumpMessages ( const Float64 __attribute__( ( unused ) ) DeltaTime )
{
    FLinuxBackend::PumpMessages();
}

Lumen::TSharedPtr<Lumen::FLinuxWindow> Lumen::FLinuxApplication::FindWindowByID ( const SDL_WindowID InWindowID ) const
{
    for ( const TSharedRef<FLinuxWindow> &Window : Windows )
    {
        if ( Window->GetOSWindowID() == InWindowID )
        {
            return Window;
        }
    }
    return nullptr;
}

#endif
