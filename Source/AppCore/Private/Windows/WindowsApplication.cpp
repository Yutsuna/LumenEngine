/**
 * @file WindowsApplication.cpp
 * @brief Implementation of the FWindowsApplication class for Windows-specific application management.
 */

#include <Windows/WindowsApplication.hpp>

#if defined( LUMEN_PLATFORM_WINDOWS )

    #include <Windows/WindowsWindow.hpp>

    #include <HAL/PlatformSignal.hpp>

    #include <Generic/GenericApplicationInput.hpp>
    #include <Generic/GenericApplicationMessageHandler.hpp>

    #include <Logging/Logger.hpp>
    #include <Logging/LoggingCategory.hpp>

    #include <Meta/Reflection.hpp>

    #include <Algo/Find.hpp>

Lumen::TAtomic<Lumen::FWindowsApplication *> Lumen::GWindowsApplication{ nullptr };

/** INFO: Private helpers for Win32 virtual-key and mouse translation. */

namespace
{

/**
 * Private
 */

const Lumen::FLogCategory LogWindowsApplication( "WindowsApplication" );

/** @brief Win32 resource-ID macros encode an integer as a pointer; the wide APIs expect LPCWSTR. */
template <typename TResource> inline LPCWSTR ToWideResource ( const TResource InResource ) noexcept
{
    return reinterpret_cast<LPCWSTR>( InResource );
}

/** @brief Translates a Win32 Virtual-Key code and lParam flags into the corresponding Lumen key. */
inline Lumen::EKeys::Type TranslateWin32VirtualKeyToEKeys ( const WPARAM InVirtualKey, const LPARAM InLParam ) noexcept
{
    const Lumen::Bool bIsExtended = ( InLParam & ( static_cast<LPARAM>( 1 ) << 24 ) ) != 0; /** NOLINT */
    const UINT ScanCode           = static_cast<UINT>( ( static_cast<Lumen::UPtr>( InLParam ) >> 16U ) & 0xFFU );

    switch ( InVirtualKey )
    {
    case '0':
        return Lumen::EKeys::Num0;
    case '1':
        return Lumen::EKeys::Num1;
    case '2':
        return Lumen::EKeys::Num2;
    case '3':
        return Lumen::EKeys::Num3;
    case '4':
        return Lumen::EKeys::Num4;
    case '5':
        return Lumen::EKeys::Num5;
    case '6':
        return Lumen::EKeys::Num6;
    case '7':
        return Lumen::EKeys::Num7;
    case '8':
        return Lumen::EKeys::Num8;
    case '9':
        return Lumen::EKeys::Num9;
    case 'A':
        return Lumen::EKeys::A;
    case 'B':
        return Lumen::EKeys::B;
    case 'C':
        return Lumen::EKeys::C;
    case 'D':
        return Lumen::EKeys::D;
    case 'E':
        return Lumen::EKeys::E;
    case 'F':
        return Lumen::EKeys::F;
    case 'G':
        return Lumen::EKeys::G;
    case 'H':
        return Lumen::EKeys::H;
    case 'I':
        return Lumen::EKeys::I;
    case 'J':
        return Lumen::EKeys::J;
    case 'K':
        return Lumen::EKeys::K;
    case 'L':
        return Lumen::EKeys::L;
    case 'M':
        return Lumen::EKeys::M;
    case 'N':
        return Lumen::EKeys::N;
    case 'O':
        return Lumen::EKeys::O;
    case 'P':
        return Lumen::EKeys::P;
    case 'Q':
        return Lumen::EKeys::Q;
    case 'R':
        return Lumen::EKeys::R;
    case 'S':
        return Lumen::EKeys::S;
    case 'T':
        return Lumen::EKeys::T;
    case 'U':
        return Lumen::EKeys::U;
    case 'V':
        return Lumen::EKeys::V;
    case 'W':
        return Lumen::EKeys::W;
    case 'X':
        return Lumen::EKeys::X;
    case 'Y':
        return Lumen::EKeys::Y;
    case 'Z':
        return Lumen::EKeys::Z;

    case VK_BACK:
        return Lumen::EKeys::Backspace;
    case VK_TAB:
        return Lumen::EKeys::Tab;
    case VK_CLEAR:
        return Lumen::EKeys::Clear;
    case VK_RETURN:
        return bIsExtended ? Lumen::EKeys::NumPadEnter : Lumen::EKeys::Enter;

    case VK_SHIFT:
    {
        const UINT ExtendedKey = MapVirtualKeyW( ScanCode, MAPVK_VSC_TO_VK_EX );
        return ( ExtendedKey == VK_RSHIFT ) ? Lumen::EKeys::RightShift : Lumen::EKeys::LeftShift;
    }
    case VK_LSHIFT:
        return Lumen::EKeys::LeftShift;
    case VK_RSHIFT:
        return Lumen::EKeys::RightShift;

    case VK_CONTROL:
        return bIsExtended ? Lumen::EKeys::RightControl : Lumen::EKeys::LeftControl;
    case VK_LCONTROL:
        return Lumen::EKeys::LeftControl;
    case VK_RCONTROL:
        return Lumen::EKeys::RightControl;

    case VK_MENU:
        return bIsExtended ? Lumen::EKeys::RightAlt : Lumen::EKeys::LeftAlt;
    case VK_LMENU:
        return Lumen::EKeys::LeftAlt;
    case VK_RMENU:
        return Lumen::EKeys::RightAlt;

    case VK_PAUSE:
        return Lumen::EKeys::Pause;
    case VK_CAPITAL:
        return Lumen::EKeys::CapsLock;
    case VK_ESCAPE:
        return Lumen::EKeys::Escape;
    case VK_SPACE:
        return Lumen::EKeys::Spacebar;
    case VK_PRIOR:
        return Lumen::EKeys::PageUp;
    case VK_NEXT:
        return Lumen::EKeys::PageDown;
    case VK_END:
        return Lumen::EKeys::End;
    case VK_HOME:
        return Lumen::EKeys::Home;
    case VK_LEFT:
        return Lumen::EKeys::Left;
    case VK_UP:
        return Lumen::EKeys::Up;
    case VK_RIGHT:
        return Lumen::EKeys::Right;
    case VK_DOWN:
        return Lumen::EKeys::Down;
    case VK_SELECT:
        return Lumen::EKeys::Select;
    case VK_EXECUTE:
        return Lumen::EKeys::Execute;
    case VK_SNAPSHOT:
        return Lumen::EKeys::PrintScreen;
    case VK_INSERT:
        return Lumen::EKeys::Insert;
    case VK_DELETE:
        return Lumen::EKeys::Delete;
    case VK_HELP:
        return Lumen::EKeys::Help;
    case VK_LWIN:
        return Lumen::EKeys::LeftGui;
    case VK_RWIN:
        return Lumen::EKeys::RightGui;
    case VK_APPS:
        return Lumen::EKeys::Application;
    case VK_SLEEP:
        return Lumen::EKeys::Sleep;

    case VK_NUMPAD0:
        return Lumen::EKeys::NumPad0;
    case VK_NUMPAD1:
        return Lumen::EKeys::NumPad1;
    case VK_NUMPAD2:
        return Lumen::EKeys::NumPad2;
    case VK_NUMPAD3:
        return Lumen::EKeys::NumPad3;
    case VK_NUMPAD4:
        return Lumen::EKeys::NumPad4;
    case VK_NUMPAD5:
        return Lumen::EKeys::NumPad5;
    case VK_NUMPAD6:
        return Lumen::EKeys::NumPad6;
    case VK_NUMPAD7:
        return Lumen::EKeys::NumPad7;
    case VK_NUMPAD8:
        return Lumen::EKeys::NumPad8;
    case VK_NUMPAD9:
        return Lumen::EKeys::NumPad9;
    case VK_MULTIPLY:
        return Lumen::EKeys::NumPadMultiply;
    case VK_ADD:
        return Lumen::EKeys::NumPadPlus;
    case VK_SEPARATOR:
        return Lumen::EKeys::Separator;
    case VK_SUBTRACT:
        return Lumen::EKeys::NumPadMinus;
    case VK_DECIMAL:
        return Lumen::EKeys::NumPadPeriod;
    case VK_DIVIDE:
        return Lumen::EKeys::NumPadDivide;

    case VK_F1:
        return Lumen::EKeys::F1;
    case VK_F2:
        return Lumen::EKeys::F2;
    case VK_F3:
        return Lumen::EKeys::F3;
    case VK_F4:
        return Lumen::EKeys::F4;
    case VK_F5:
        return Lumen::EKeys::F5;
    case VK_F6:
        return Lumen::EKeys::F6;
    case VK_F7:
        return Lumen::EKeys::F7;
    case VK_F8:
        return Lumen::EKeys::F8;
    case VK_F9:
        return Lumen::EKeys::F9;
    case VK_F10:
        return Lumen::EKeys::F10;
    case VK_F11:
        return Lumen::EKeys::F11;
    case VK_F12:
        return Lumen::EKeys::F12;
    case VK_F13:
        return Lumen::EKeys::F13;
    case VK_F14:
        return Lumen::EKeys::F14;
    case VK_F15:
        return Lumen::EKeys::F15;
    case VK_F16:
        return Lumen::EKeys::F16;
    case VK_F17:
        return Lumen::EKeys::F17;
    case VK_F18:
        return Lumen::EKeys::F18;
    case VK_F19:
        return Lumen::EKeys::F19;
    case VK_F20:
        return Lumen::EKeys::F20;
    case VK_F21:
        return Lumen::EKeys::F21;
    case VK_F22:
        return Lumen::EKeys::F22;
    case VK_F23:
        return Lumen::EKeys::F23;
    case VK_F24:
        return Lumen::EKeys::F24;

    case VK_NUMLOCK:
        return Lumen::EKeys::NumLock;
    case VK_SCROLL:
        return Lumen::EKeys::ScrollLock;

    case VK_BROWSER_BACK:
        return Lumen::EKeys::AcBack;
    case VK_BROWSER_FORWARD:
        return Lumen::EKeys::AcForward;
    case VK_BROWSER_REFRESH:
        return Lumen::EKeys::AcRefresh;
    case VK_BROWSER_STOP:
        return Lumen::EKeys::AcStop;
    case VK_BROWSER_SEARCH:
        return Lumen::EKeys::AcSearch;
    case VK_BROWSER_FAVORITES:
        return Lumen::EKeys::AcBookmarks;
    case VK_BROWSER_HOME:
        return Lumen::EKeys::AcHome;

    case VK_VOLUME_MUTE:
        return Lumen::EKeys::Mute;
    case VK_VOLUME_DOWN:
        return Lumen::EKeys::VolumeDown;
    case VK_VOLUME_UP:
        return Lumen::EKeys::VolumeUp;
    case VK_MEDIA_NEXT_TRACK:
        return Lumen::EKeys::MediaNextTrack;
    case VK_MEDIA_PREV_TRACK:
        return Lumen::EKeys::MediaPreviousTrack;
    case VK_MEDIA_STOP:
        return Lumen::EKeys::MediaStop;
    case VK_MEDIA_PLAY_PAUSE:
        return Lumen::EKeys::MediaPlayPause;

    case VK_OEM_1:
        return Lumen::EKeys::Semicolon;
    case VK_OEM_PLUS:
        return Lumen::EKeys::Equals;
    case VK_OEM_COMMA:
        return Lumen::EKeys::Comma;
    case VK_OEM_MINUS:
        return Lumen::EKeys::Minus;
    case VK_OEM_PERIOD:
        return Lumen::EKeys::Period;
    case VK_OEM_2:
        return Lumen::EKeys::Slash;
    case VK_OEM_3:
        return Lumen::EKeys::GraveAccent;
    case VK_OEM_4:
        return Lumen::EKeys::LeftBracket;
    case VK_OEM_5:
    case VK_OEM_102:
        return Lumen::EKeys::Backslash;
    case VK_OEM_6:
        return Lumen::EKeys::RightBracket;
    case VK_OEM_7:
        return Lumen::EKeys::Apostrophe;

    case VK_CANCEL:
        return Lumen::EKeys::Cancel;
    case VK_PLAY:
        return Lumen::EKeys::MediaPlay;
    case VK_CRSEL:
        return Lumen::EKeys::CrSel;
    case VK_EXSEL:
        return Lumen::EKeys::ExSel;

    default:
        return Lumen::EKeys::Unknown;
    }
}

/** @brief Translates a Win32 XBUTTON parameter into the corresponding Lumen mouse button. */
inline Lumen::EMouseButton::Type TranslateWin32XButtonToEMouseButton ( const WPARAM InWParam ) noexcept
{
    const WORD ButtonFlag = HIWORD( InWParam );
    switch ( ButtonFlag )
    {
    case XBUTTON1:
        return Lumen::EMouseButton::Button4;
    case XBUTTON2:
        return Lumen::EMouseButton::Button5;
    default:
        return Lumen::EMouseButton::Invalid;
    }
}

} // namespace

/**
 * Public
 */

Lumen::FWindowsApplication::FWindowsApplication () noexcept : InstanceHandle( GetModuleHandleW( nullptr ) )
{
    /** Ctor */
}

Lumen::FWindowsApplication::~FWindowsApplication () noexcept
{
    FWindowsApplication::Shutdown();

    if ( GWindowsApplication.load( std::memory_order_relaxed ) == this )
    {
        GWindowsApplication.store( nullptr, std::memory_order_relaxed );
    }
}

Lumen::TSharedRef<Lumen::FGenericWindow> Lumen::FWindowsApplication::MakeWindow () noexcept
{
    return FWindowsWindow::Make();
}

Lumen::TSharedPtr<Lumen::FGenericApplication> Lumen::FWindowsApplication::MakeWindowsApplication () noexcept
{
    /** Use MakeShared to align the control block and pointer cleanly together */
    const TSharedPtr<FWindowsApplication> Application = MakeShared<FWindowsApplication>();
    if ( not Application->EnsureWindowClassRegistered() )
    {
        LUMEN_LOG_FATAL( LogWindowsApplication, "Failed to register Win32 window class for Windows application (error {}).",
                         GetLastError() );
        /** INFO: unused as LUMEN LOG FATAL will raise a terminate signal after logging the message */
        return nullptr;
    }

    GWindowsApplication.store( Application.Get(), std::memory_order_release );
    GPlatformApplication = StaticCastSharedPtr<FGenericApplication>( Application );

    return GPlatformApplication;
}

void Lumen::FWindowsApplication::InitializeWindow ( const TSharedRef<FGenericWindow> &InWindow,
                                                    const TSharedRef<FGenericWindowDescription> &InDescription,
                                                    const TSharedPtr<FGenericWindow> &InParentWindow,
                                                    const Bool bShowImmediately ) noexcept
{
    /** INFO: Reject anything that is not the Windows-backed window this factory produces. */
    if ( dynamic_cast<FWindowsWindow *>( InWindow.Get() ) == nullptr ) [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogWindowsApplication, "InitializeWindow requires a Windows window instance." );
        return;
    }

    if ( not EnsureWindowClassRegistered() ) [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogWindowsApplication, "Failed to register Win32 window class (error {}).", GetLastError() );
        return;
    }

    const TSharedRef<FWindowsWindow> WindowsWindow = StaticCastSharedRef<FWindowsWindow>( InWindow );
    const TExpected<void, EAppCoreError::Type> InitResult =
        WindowsWindow->Initialize( this, InDescription, InParentWindow, bShowImmediately );

    if ( not InitResult ) [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogWindowsApplication, "Failed to initialize Windows window: {}", Meta::EnumName( InitResult.Error() ) );
        return;
    }

    Windows.push_back( WindowsWindow );
    if ( not MainWindow.IsValid() )
    {
        MainWindow = InWindow;
    }
}

void Lumen::FWindowsApplication::PumpMessages ( const Float64 LUMEN_UNUSED DeltaTime ) noexcept
{
    MSG Message{};
    while ( PeekMessageW( &Message, nullptr, 0U, 0U, PM_REMOVE ) != FALSE )
    {
        if ( Message.message == WM_QUIT )
        {
            if ( MessageHandler.IsValid() )
            {
                MessageHandler->OnQuit();
            }
            HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Terminate );
            break;
        }

        TranslateMessage( &Message );
        DispatchMessageW( &Message );
    }
}

void Lumen::FWindowsApplication::Shutdown () noexcept
{
    /** INFO: Destroy all tracked OS windows before unregistering the Win32 window class. */
    for ( const TSharedRef<FWindowsWindow> &Window : Windows )
    {
        Window->Destroy();
    }
    Windows.clear();
    MainWindow.Reset();
    MessageHandler.Reset();

    if ( bWindowClassRegistered and InstanceHandle != nullptr )
    {
        UnregisterClassW( FWindowsWindow::GetWindowClassName(), InstanceHandle );
        bWindowClassRegistered = false;
    }

    if ( GWindowsApplication.load( std::memory_order_relaxed ) == this )
    {
        GWindowsApplication.store( nullptr, std::memory_order_release );
    }
}

HINSTANCE Lumen::FWindowsApplication::GetOSInstanceHandle () const noexcept
{
    return InstanceHandle;
}

LRESULT Lumen::FWindowsApplication::ProcessMessage ( HWND InWindowHandle,
                                                     const UINT InMessage,
                                                     const WPARAM InWParam,
                                                     const LPARAM InLParam ) noexcept
{
    if ( not MessageHandler.IsValid() )
    {
        return DefWindowProcW( InWindowHandle, InMessage, InWParam, InLParam );
    }

    const TOptional<TSharedRef<FWindowsWindow>> CurrentWindow = FindWindowByHandle( InWindowHandle );
    if ( not CurrentWindow.HasValue() )
    {
        return DefWindowProcW( InWindowHandle, InMessage, InWParam, InLParam );
    }

    const TSharedRef<FWindowsWindow> &Window = *CurrentWindow;
    switch ( InMessage )
    {
    case WM_CLOSE:
        MessageHandler->OnRequestExit();
        return 0;

    case WM_MOVE:
        if ( Window->Description.IsValid() )
        {
            Window->Description->Position = Window->GetWindowPosition();
        }
        MessageHandler->OnWindowMoved( Window );
        return 0;

    case WM_SIZE:
        if ( Window->Description.IsValid() )
        {
            Window->Description->Size = Window->GetWindowSize();
        }
        MessageHandler->OnWindowResized( Window );
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT PaintStruct{};
        BeginPaint( InWindowHandle, &PaintStruct );
        MessageHandler->OnOSPaint( Window );
        EndPaint( InWindowHandle, &PaintStruct );
        return 0;
    }

    case WM_ACTIVATE:
        MessageHandler->OnWindowActivationChanged( Window );
        break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const EKeys::Type KeyType = TranslateWin32VirtualKeyToEKeys( InWParam, InLParam );
        const Bool bIsRepeated    = ( InLParam & ( static_cast<LPARAM>( 1 ) << 30 ) ) != 0; /** NOLINT */
        MessageHandler->OnKeyDown( KeyType, bIsRepeated );
        if ( InMessage == WM_SYSKEYDOWN )
        {
            break;
        }
        return 0;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        const EKeys::Type KeyType = TranslateWin32VirtualKeyToEKeys( InWParam, InLParam );
        MessageHandler->OnKeyUp( KeyType, false );
        if ( InMessage == WM_SYSKEYUP )
        {
            break;
        }
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        const Int32 MouseX = static_cast<Int32>( static_cast<Int16>( LOWORD( InLParam ) ) );
        const Int32 MouseY = static_cast<Int32>( static_cast<Int16>( HIWORD( InLParam ) ) );
        MessageHandler->OnMouseMove( MouseX, MouseY );
        return 0;
    }

    case WM_LBUTTONDOWN:
        MessageHandler->OnMouseDown( EMouseButton::Left );
        return 0;

    case WM_LBUTTONUP:
        MessageHandler->OnMouseUp( EMouseButton::Left );
        return 0;

    case WM_LBUTTONDBLCLK:
        MessageHandler->OnMouseDoubleClick( EMouseButton::Left );
        return 0;

    case WM_MBUTTONDOWN:
        MessageHandler->OnMouseDown( EMouseButton::Middle );
        return 0;

    case WM_MBUTTONUP:
        MessageHandler->OnMouseUp( EMouseButton::Middle );
        return 0;

    case WM_MBUTTONDBLCLK:
        MessageHandler->OnMouseDoubleClick( EMouseButton::Middle );
        return 0;

    case WM_RBUTTONDOWN:
        MessageHandler->OnMouseDown( EMouseButton::Right );
        return 0;

    case WM_RBUTTONUP:
        MessageHandler->OnMouseUp( EMouseButton::Right );
        return 0;

    case WM_RBUTTONDBLCLK:
        MessageHandler->OnMouseDoubleClick( EMouseButton::Right );
        return 0;

    case WM_XBUTTONDOWN:
    {
        const EMouseButton::Type Button = TranslateWin32XButtonToEMouseButton( InWParam );
        if ( Button != EMouseButton::Invalid )
        {
            MessageHandler->OnMouseDown( Button );
        }
        return TRUE;
    }

    case WM_XBUTTONUP:
    {
        const EMouseButton::Type Button = TranslateWin32XButtonToEMouseButton( InWParam );
        if ( Button != EMouseButton::Invalid )
        {
            MessageHandler->OnMouseUp( Button );
        }
        return TRUE;
    }

    case WM_XBUTTONDBLCLK:
    {
        const EMouseButton::Type Button = TranslateWin32XButtonToEMouseButton( InWParam );
        if ( Button != EMouseButton::Invalid )
        {
            MessageHandler->OnMouseDoubleClick( Button );
        }
        return TRUE;
    }

    case WM_MOUSEWHEEL:
    {
        const Int16 WheelDelta  = static_cast<Int16>( HIWORD( InWParam ) );
        const Int32 ScaledDelta = static_cast<Int32>( WheelDelta ) * 100 / WHEEL_DELTA;
        MessageHandler->OnMouseWheel( ScaledDelta );
        return 0;
    }

    default:
        break;
    }

    return DefWindowProcW( InWindowHandle, InMessage, InWParam, InLParam );
}

/**
 * Private
 */

LRESULT CALLBACK Lumen::FWindowsApplication::AppWndProc ( HWND InWindowHandle,
                                                          const UINT InMessage,
                                                          const WPARAM InWParam,
                                                          const LPARAM InLParam ) noexcept
{
    if ( InMessage == WM_NCCREATE )
    {
        /** NOLINTNEXTLINE(performance-no-int-to-ptr) */
        const auto *const CreateStruct = reinterpret_cast<const CREATESTRUCTW *>( InLParam );
        if ( CreateStruct != nullptr and CreateStruct->lpCreateParams != nullptr )
        {
            SetWindowLongPtrW( InWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( CreateStruct->lpCreateParams ) );
        }
        return DefWindowProcW( InWindowHandle, InMessage, InWParam, InLParam );
    }

    /** NOLINTNEXTLINE(performance-no-int-to-ptr) */
    auto *const RawWindow = reinterpret_cast<FWindowsWindow *>( GetWindowLongPtrW( InWindowHandle, GWLP_USERDATA ) );
    if ( InMessage == WM_NCDESTROY )
    {
        if ( RawWindow != nullptr )
        {
            RawWindow->WindowHandle = nullptr;
            SetWindowLongPtrW( InWindowHandle, GWLP_USERDATA, 0 );
        }
        return DefWindowProcW( InWindowHandle, InMessage, InWParam, InLParam );
    }

    FWindowsApplication *Application = nullptr;
    if ( RawWindow != nullptr and RawWindow->WindowsApplication != nullptr )
    {
        Application = RawWindow->WindowsApplication;
    }
    else
    {
        Application = GWindowsApplication.load( std::memory_order_acquire );
    }

    if ( Application != nullptr ) [[likely]]
    {
        return Application->ProcessMessage( InWindowHandle, InMessage, InWParam, InLParam );
    }

    return DefWindowProcW( InWindowHandle, InMessage, InWParam, InLParam );
}

Lumen::Bool Lumen::FWindowsApplication::EnsureWindowClassRegistered () noexcept
{
    if ( bWindowClassRegistered ) [[likely]]
    {
        return true;
    }

    if ( InstanceHandle == nullptr )
    {
        InstanceHandle = GetModuleHandleW( nullptr );
    }

    WNDCLASSEXW ExistingClass{};
    ExistingClass.cbSize = sizeof( WNDCLASSEXW );
    if ( GetClassInfoExW( InstanceHandle, FWindowsWindow::GetWindowClassName(), &ExistingClass ) != FALSE )
    {
        bWindowClassRegistered = true;
        return true;
    }

    WNDCLASSEXW WindowClass{};
    WindowClass.cbSize        = sizeof( WNDCLASSEXW );
    WindowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;
    WindowClass.lpfnWndProc   = &FWindowsApplication::AppWndProc;
    WindowClass.cbClsExtra    = 0;
    WindowClass.cbWndExtra    = 0;
    WindowClass.hInstance     = InstanceHandle;
    WindowClass.hIcon         = LoadIconW( nullptr, ToWideResource( IDI_APPLICATION ) );
    WindowClass.hCursor       = LoadCursorW( nullptr, ToWideResource( IDC_ARROW ) );
    WindowClass.hbrBackground = nullptr;
    WindowClass.lpszMenuName  = nullptr;
    WindowClass.lpszClassName = FWindowsWindow::GetWindowClassName();
    WindowClass.hIconSm       = LoadIconW( nullptr, ToWideResource( IDI_APPLICATION ) );

    if ( RegisterClassExW( &WindowClass ) == 0 )
    {
        if ( GetLastError() != ERROR_CLASS_ALREADY_EXISTS )
        {
            return false;
        }
    }

    bWindowClassRegistered = true;
    return true;
}

Lumen::TOptional<Lumen::TSharedRef<Lumen::FWindowsWindow>>
Lumen::FWindowsApplication::FindWindowByHandle ( HWND InWindowHandle ) const noexcept
{
    if ( InWindowHandle == nullptr )
    {
        return {};
    }

    return Algo::FindIfOrNone( Windows, [&] ( const TSharedRef<FWindowsWindow> &Window ) noexcept -> Bool
    {
        return Window->GetOSWindowHandle() == InWindowHandle;
    } );
}

#endif
