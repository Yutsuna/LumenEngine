/**
 * @file LinuxApplication.cpp
 * @brief Implementation of the FLinuxApplication class for Linux-specific application management.
 */

#include "Linux/LinuxApplication.hpp"
#include "Linux/LinuxBackend.hpp"
#include "Linux/LinuxWindow.hpp"

#include "Generic/GenericApplicationInput.hpp"
#include "Generic/GenericApplicationMessageHandler.hpp"

#include "Logging/LoggingCategory.hpp"
#include "SDL3/SDL_events.h"

#include <Logging/Logger.hpp>

#include <SDL3/SDL.h>

LumenEngine::FLinuxApplication *LumenEngine::GLinuxApplication = nullptr;

namespace
{

LumenEngine::FLogCategory LogLinuxApplication( "LinuxApplication" );

}

LumenEngine::FLinuxApplication::FLinuxApplication () : FGenericApplication()
{
    /* Empty */
}

LumenEngine::FLinuxApplication::~FLinuxApplication ()
{
    FLinuxBackend::ShutdownSDL();
}

LumenEngine::TSharedRef<LumenEngine::FGenericWindow> LumenEngine::FLinuxApplication::MakeWindow ()
{
    return FLinuxWindow::Make();
}

LumenEngine::TSharedPtr<LumenEngine::FGenericApplication> LumenEngine::FLinuxApplication::CreateLinuxApplication ()
{
    if ( not FLinuxBackend::InitializeSDL() )
    {
        LUMEN_LOG_FATAL( LogLinuxApplication, "Failed to initialize SDL library for Linux application: {}", SDL_GetError() );
        /** INFO: unused as LUMEN LOG FATAL with raise a terminate signal after logging the message */
        return nullptr;
    }

    FLinuxApplication *Application = new FLinuxApplication();
    GLinuxApplication              = Application;

    return MakeShareable( Application );
}

void LumenEngine::FLinuxApplication::InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                                                        const TSharedRef<FGenericWindowDescription> &InDescription,
                                                        const TSharedPtr<FGenericWindow> &InParentWindow,
                                                        const Bool bShowImmediately )
{
    const TSharedRef<FLinuxWindow> LinuxWindow  = StaticCastSharedRef<FLinuxWindow>( InWindow );
    const TSharedPtr<FLinuxWindow> ParentWindow = StaticCastSharedPtr<FLinuxWindow>( InParentWindow );

    LinuxWindow->Initialize( this, InDescription, InParentWindow, bShowImmediately );
    Windows.push_back( LinuxWindow );
}

namespace LumenEngine
{

namespace
{

    static inline void SendKeyDownEvent ( TSharedPtr<FGenericApplicationMessageHandler> MessageHandler, const SDL_KeyboardEvent &KeyboardEvent )
    {
        const SDL_Keycode KeyCode = KeyboardEvent.key;
        const EKeys::Type KeyType = TranslateSDLKeyCodeToEKeys( KeyCode );
        const Bool bIsRepeated    = KeyboardEvent.repeat != 0;

        MessageHandler->OnKeyDown( KeyType, bIsRepeated );
    }

    static inline void SendKeyUpEvent ( TSharedPtr<FGenericApplicationMessageHandler> MessageHandler, const SDL_KeyboardEvent &KeyboardEvent )
    {
        const SDL_Keycode KeyCode = KeyboardEvent.key;
        const EKeys::Type KeyType = TranslateSDLKeyCodeToEKeys( KeyCode );

        MessageHandler->OnKeyUp( KeyType );
    }

    static inline void SendMouseButtonDownEvent ( TSharedPtr<FGenericApplicationMessageHandler> MessageHandler, const SDL_MouseButtonEvent &MouseButtonEvent )
    {
        const EMouseButton::Type ButtonType = TranslateSDLMouseButtonToEMouseButton( MouseButtonEvent.button );

        MessageHandler->OnMouseDown( ButtonType );
    }

    static inline void SendMouseButtonUpEvent ( TSharedPtr<FGenericApplicationMessageHandler> MessageHandler, const SDL_MouseButtonEvent &MouseButtonEvent )
    {
        const EMouseButton::Type ButtonType = TranslateSDLMouseButtonToEMouseButton( MouseButtonEvent.button );

        MessageHandler->OnMouseUp( ButtonType );
    }

} // namespace

} // namespace LumenEngine

void LumenEngine::FLinuxApplication::AddPendingEvent ( const SDL_Event &InEvent )
{
    TSharedPtr<FLinuxWindow> CurrentWindow = FindWindowByID( InEvent.window.windowID );
    SDL_Window *WindowHandle               = nullptr;

    if ( CurrentWindow.IsValid() )
    {
        WindowHandle = CurrentWindow->GetOSWindowHandle();
    }
    if ( not WindowHandle )
    {
        return;
    }

    switch ( InEvent.type )
    {
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

void LumenEngine::FLinuxApplication::PumpMessages ( const Float32 __attribute__( ( unused ) ) DeltaTime )
{
    FLinuxBackend::PumpMessages();
}

LumenEngine::TSharedPtr<LumenEngine::FLinuxWindow> LumenEngine::FLinuxApplication::FindWindowByID ( const SDL_WindowID InWindowID ) const
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
