/**
 * @file WindowsWindow.cpp
 * @brief Implementation of the FWindowsWindow class for Windows-specific window management.
 */

#include <Windows/WindowsWindow.hpp>

#if defined( LUMEN_PLATFORM_WINDOWS )

    #include <Windows/WindowsApplication.hpp>

    #include <Logging/Logger.hpp>
    #include <Templates/Unexpected.hpp>

    #include <string>

namespace
{

const Lumen::FLogCategory LogWindowsWindow( "WindowsWindow" );

constexpr const Lumen::WideChar *const LumenWindowClassName = L"LumenWindow";

/** @brief Converts a UTF-8 C string to a UTF-16 std::wstring for Win32 wide APIs. */
inline std::wstring Utf8ToWide ( const Lumen::AnsiChar *const InUtf8 ) noexcept
{
    if ( InUtf8 == nullptr or *InUtf8 == '\0' )
    {
        return {};
    }

    const int RequiredChars = MultiByteToWideChar( CP_UTF8, 0U, InUtf8, -1, nullptr, 0 );
    if ( RequiredChars <= 1 )
    {
        return {};
    }

    std::wstring Result( static_cast<std::size_t>( RequiredChars - 1 ), L'\0' );
    if ( MultiByteToWideChar( CP_UTF8, 0U, InUtf8, -1, Result.data(), RequiredChars ) <= 0 )
    {
        return {};
    }

    return Result;
}

/** @brief Computes the Win32 window style flags for standard windowed mode. */
inline DWORD GetWindowedStyle ( const Lumen::Bool bIsResizable ) noexcept
{
    DWORD Style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    if ( bIsResizable )
    {
        Style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    }
    return Style;
}

/** @brief Retrieves the monitor rectangle covering a target point, falling back to the primary screen. */
inline RECT GetMonitorRectFromPoint ( const Lumen::Int32 InX, const Lumen::Int32 InY ) noexcept
{
    const POINT Anchor{ static_cast<LONG>( InX ), static_cast<LONG>( InY ) };
    const HMONITOR Monitor = MonitorFromPoint( Anchor, MONITOR_DEFAULTTONEAREST );
    MONITORINFO MonitorInfo{};
    MonitorInfo.cbSize = sizeof( MONITORINFO );
    if ( Monitor != nullptr and GetMonitorInfoW( Monitor, &MonitorInfo ) != FALSE )
    {
        return MonitorInfo.rcMonitor;
    }

    return RECT{ 0, 0, GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ) };
}

/** @brief Retrieves the monitor rectangle hosting the given window, falling back to the primary screen. */
inline RECT GetMonitorRectFromWindow ( const HWND InWindowHandle ) noexcept
{
    const HMONITOR Monitor = MonitorFromWindow( InWindowHandle, MONITOR_DEFAULTTONEAREST );
    MONITORINFO MonitorInfo{};
    MonitorInfo.cbSize = sizeof( MONITORINFO );
    if ( Monitor != nullptr and GetMonitorInfoW( Monitor, &MonitorInfo ) != FALSE )
    {
        return MonitorInfo.rcMonitor;
    }

    return RECT{ 0, 0, GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ) };
}

} // namespace

/** INFO: FWindowsWindow lifetime implementation. */

Lumen::FWindowsWindow::~FWindowsWindow () noexcept
{
    this->FWindowsWindow::Destroy();
}

/** INFO: Factory for Windows-backed windows. */

Lumen::TSharedRef<Lumen::FWindowsWindow> Lumen::FWindowsWindow::Make () noexcept
{
    /** NOLINTNEXTLINE(bugprone-unhandled-exception-at-new) */
    return MakeShareable<FWindowsWindow>( new FWindowsWindow() );
}

Lumen::TExpected<void, Lumen::EAppCoreError::Type>
Lumen::FWindowsWindow::Initialize ( FWindowsApplication *InApplication,
                                    const TSharedRef<FGenericWindowDescription> &InDescription,
                                    const TSharedPtr<FGenericWindow> &InParentWindow,
                                    const Bool bShowImmediately ) noexcept
{
    const HINSTANCE ResolvedInstance = ( InApplication != nullptr and InApplication->GetOSInstanceHandle() != nullptr )
                                           ? InApplication->GetOSInstanceHandle()
                                           : GetModuleHandleW( nullptr );

    HWND ParentHWND = nullptr;
    if ( InParentWindow.IsValid() )
    {
        const auto *const WindowsParent = dynamic_cast<const FWindowsWindow *>( InParentWindow.Get() );
        if ( WindowsParent != nullptr )
        {
            ParentHWND = WindowsParent->GetOSWindowHandle();
        }
    }

    DWORD WindowStyle         = 0U;
    const DWORD WindowExStyle = WS_EX_APPWINDOW;
    Int32 WindowPosX          = InDescription->Position.X;
    Int32 WindowPosY          = InDescription->Position.Y;
    Int32 OuterWidth          = InDescription->Size.X;
    Int32 OuterHeight         = InDescription->Size.Y;

    if ( InDescription->WindowMode == EWindowMode::Windowed )
    {
        WindowStyle = GetWindowedStyle( InDescription->bIsResizable );
        RECT AdjustedRect{ 0, 0, static_cast<LONG>( InDescription->Size.X ), static_cast<LONG>( InDescription->Size.Y ) };
        if ( AdjustWindowRectEx( &AdjustedRect, WindowStyle, FALSE, WindowExStyle ) != FALSE )
        {
            OuterWidth  = static_cast<Int32>( AdjustedRect.right - AdjustedRect.left );
            OuterHeight = static_cast<Int32>( AdjustedRect.bottom - AdjustedRect.top );
        }
    }
    else
    {
        WindowStyle            = WS_POPUP;
        const RECT MonitorRect = GetMonitorRectFromPoint( InDescription->Position.X, InDescription->Position.Y );
        WindowPosX             = static_cast<Int32>( MonitorRect.left );
        WindowPosY             = static_cast<Int32>( MonitorRect.top );
        OuterWidth             = static_cast<Int32>( MonitorRect.right - MonitorRect.left );
        OuterHeight            = static_cast<Int32>( MonitorRect.bottom - MonitorRect.top );
    }

    WindowsApplication = InApplication;
    InstanceHandle     = ResolvedInstance;
    Description        = InDescription;
    ParentWindow       = InParentWindow;
    CurrentWindowMode  = InDescription->WindowMode;

    const std::wstring WideTitle = Utf8ToWide( InDescription->Title.c_str() );
    const HWND CreatedHandle =
        CreateWindowExW( WindowExStyle, GetWindowClassName(), WideTitle.c_str(), WindowStyle, static_cast<int>( WindowPosX ),
                         static_cast<int>( WindowPosY ), static_cast<int>( OuterWidth ), static_cast<int>( OuterHeight ),
                         ParentHWND, nullptr, ResolvedInstance, this );

    if ( CreatedHandle == nullptr )
    {
        LUMEN_LOG_ERROR( LogWindowsWindow, "Failed to create Win32 window (error {}).", GetLastError() );
        WindowsApplication = nullptr;
        InstanceHandle     = nullptr;
        Description.Reset();
        ParentWindow.Reset();
        return MakeUnexpected( EAppCoreError::Type::WindowCreationFailed );
    }

    WindowHandle = CreatedHandle;

    if ( InDescription->bIsVisible and bShowImmediately )
    {
        ShowWindow( WindowHandle, SW_SHOW );
        UpdateWindow( WindowHandle );
    }

    return {};
}

/** INFO: FGenericWindow overrides. */

void Lumen::FWindowsWindow::Destroy () noexcept
{
    if ( WindowHandle != nullptr )
    {
        const HWND HandleToDestroy = WindowHandle;
        WindowHandle               = nullptr;
        DestroyWindow( HandleToDestroy );
    }
}

void Lumen::FWindowsWindow::Show () noexcept
{
    if ( WindowHandle != nullptr )
    {
        ShowWindow( WindowHandle, SW_SHOW );
        if ( Description.IsValid() )
        {
            Description->bIsVisible = true;
        }
    }
}

void Lumen::FWindowsWindow::Minimize () noexcept
{
    if ( WindowHandle != nullptr )
    {
        ShowWindow( WindowHandle, SW_MINIMIZE );
    }
}

void Lumen::FWindowsWindow::Maximize () noexcept
{
    if ( WindowHandle != nullptr )
    {
        ShowWindow( WindowHandle, SW_MAXIMIZE );
        if ( Description.IsValid() )
        {
            Description->bIsVisible = true;
        }
    }
}

void Lumen::FWindowsWindow::Hide () noexcept
{
    if ( WindowHandle != nullptr )
    {
        ShowWindow( WindowHandle, SW_HIDE );
        if ( Description.IsValid() )
        {
            Description->bIsVisible = false;
        }
    }
}

void Lumen::FWindowsWindow::SetWindowMode ( const EWindowMode::Type InWindowMode ) noexcept
{
    if ( WindowHandle == nullptr or InWindowMode == CurrentWindowMode )
    {
        return;
    }

    const Bool bTargetIsFullscreen =
        ( InWindowMode == EWindowMode::Fullscreen ) or ( InWindowMode == EWindowMode::WindowedFullscreen );
    const Bool bCurrentIsWindowed = ( CurrentWindowMode == EWindowMode::Windowed );

    if ( bTargetIsFullscreen and bCurrentIsWindowed )
    {
        SavedWindowPlacement.length = sizeof( WINDOWPLACEMENT );
        bHasSavedPlacement          = ( GetWindowPlacement( WindowHandle, &SavedWindowPlacement ) != FALSE );
        SavedWindowStyle            = static_cast<DWORD>( GetWindowLongPtrW( WindowHandle, GWL_STYLE ) );
        SavedWindowExStyle          = static_cast<DWORD>( GetWindowLongPtrW( WindowHandle, GWL_EXSTYLE ) );

        const DWORD VisibleBit = SavedWindowStyle & WS_VISIBLE;
        SetWindowLongPtrW( WindowHandle, GWL_STYLE, static_cast<LONG_PTR>( WS_POPUP | VisibleBit ) );

        const RECT MonitorRect = GetMonitorRectFromWindow( WindowHandle );
        SetWindowPos( WindowHandle, HWND_TOP, static_cast<int>( MonitorRect.left ), static_cast<int>( MonitorRect.top ),
                      static_cast<int>( MonitorRect.right - MonitorRect.left ),
                      static_cast<int>( MonitorRect.bottom - MonitorRect.top ), SWP_NOOWNERZORDER | SWP_FRAMECHANGED );
    }
    else if ( InWindowMode == EWindowMode::Windowed )
    {
        const Bool bIsCurrentlyVisible = IsWindowVisible( WindowHandle ) != FALSE;
        const DWORD CurrentVisibleBit  = bIsCurrentlyVisible ? WS_VISIBLE : 0U;

        if ( bHasSavedPlacement )
        {
            const DWORD RestoredStyle = ( SavedWindowStyle & ~static_cast<DWORD>( WS_VISIBLE ) ) | CurrentVisibleBit;
            SetWindowLongPtrW( WindowHandle, GWL_STYLE, static_cast<LONG_PTR>( RestoredStyle ) );

            if ( not bIsCurrentlyVisible )
            {
                SavedWindowPlacement.showCmd = SW_HIDE;
            }
            else if ( SavedWindowPlacement.showCmd == SW_HIDE )
            {
                SavedWindowPlacement.showCmd = SW_SHOWNORMAL;
            }

            SetWindowPlacement( WindowHandle, &SavedWindowPlacement );
            SetWindowPos( WindowHandle, nullptr, 0, 0, 0, 0,
                          SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED );
        }
        else
        {
            const Bool bIsResizable    = Description.IsValid() ? Description->bIsResizable : true;
            const DWORD TargetStyle    = GetWindowedStyle( bIsResizable ) | CurrentVisibleBit;
            const DWORD CurrentExStyle = static_cast<DWORD>( GetWindowLongPtrW( WindowHandle, GWL_EXSTYLE ) );
            SetWindowLongPtrW( WindowHandle, GWL_STYLE, static_cast<LONG_PTR>( TargetStyle ) );

            const Int32 PosX         = Description.IsValid() ? Description->Position.X : DefaultWindowPosX;
            const Int32 PosY         = Description.IsValid() ? Description->Position.Y : DefaultWindowPosY;
            const Int32 ClientWidth  = Description.IsValid() ? Description->Size.X : DefaultWindowWidth;
            const Int32 ClientHeight = Description.IsValid() ? Description->Size.Y : DefaultWindowHeight;

            RECT AdjustedRect{ 0, 0, static_cast<LONG>( ClientWidth ), static_cast<LONG>( ClientHeight ) };
            AdjustWindowRectEx( &AdjustedRect, TargetStyle, FALSE, CurrentExStyle );

            SetWindowPos( WindowHandle, HWND_NOTOPMOST, static_cast<int>( PosX ), static_cast<int>( PosY ),
                          static_cast<int>( AdjustedRect.right - AdjustedRect.left ),
                          static_cast<int>( AdjustedRect.bottom - AdjustedRect.top ),
                          SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED );
        }
    }

    CurrentWindowMode = InWindowMode;
    if ( Description.IsValid() )
    {
        Description->WindowMode = InWindowMode;
        Description->Size       = GetWindowSize();
        Description->Position   = GetWindowPosition();
    }
}

Lumen::EWindowMode::Type Lumen::FWindowsWindow::GetWindowMode () const noexcept
{
    return CurrentWindowMode;
}

Lumen::Bool Lumen::FWindowsWindow::IsMaximized () const noexcept
{
    return ( WindowHandle != nullptr ) and ( IsZoomed( WindowHandle ) != FALSE );
}

Lumen::Bool Lumen::FWindowsWindow::IsMinimized () const noexcept
{
    return ( WindowHandle != nullptr ) and ( IsIconic( WindowHandle ) != FALSE );
}

Lumen::Bool Lumen::FWindowsWindow::IsVisible () const noexcept
{
    return ( WindowHandle != nullptr ) and ( IsWindowVisible( WindowHandle ) != FALSE );
}

void Lumen::FWindowsWindow::SetWindowFocus () noexcept
{
    if ( WindowHandle != nullptr )
    {
        SetForegroundWindow( WindowHandle );
        SetFocus( WindowHandle );
    }
}

void Lumen::FWindowsWindow::SetOpacity ( const Float32 InOpacity ) noexcept
{
    if ( WindowHandle == nullptr )
    {
        return;
    }

    const Float32 ClampedOpacity = ( InOpacity < 0.0F ) ? 0.0F : ( ( InOpacity > 1.0F ) ? 1.0F : InOpacity );
    const BYTE Alpha             = static_cast<BYTE>( ClampedOpacity * 255.0F );
    const LONG_PTR ExStyle       = GetWindowLongPtrW( WindowHandle, GWL_EXSTYLE );

    if ( Alpha < 255U )
    {
        if ( ( ExStyle & WS_EX_LAYERED ) == 0 )
        {
            SetWindowLongPtrW( WindowHandle, GWL_EXSTYLE, ExStyle | WS_EX_LAYERED );
        }
        SetLayeredWindowAttributes( WindowHandle, 0U, Alpha, LWA_ALPHA );
    }
    else if ( ( ExStyle & WS_EX_LAYERED ) != 0 )
    {
        SetWindowLongPtrW( WindowHandle, GWL_EXSTYLE, ExStyle & ~static_cast<LONG_PTR>( WS_EX_LAYERED ) );
    }
}

void Lumen::FWindowsWindow::EnableInput ( const Bool bEnable ) noexcept
{
    if ( WindowHandle != nullptr )
    {
        EnableWindow( WindowHandle, bEnable ? TRUE : FALSE );
    }
}

Lumen::Int32 Lumen::FWindowsWindow::GetOSWindowBorderSize () const noexcept
{
    if ( WindowHandle == nullptr )
    {
        return 0;
    }

    const DWORD Style   = static_cast<DWORD>( GetWindowLongPtrW( WindowHandle, GWL_STYLE ) );
    const DWORD ExStyle = static_cast<DWORD>( GetWindowLongPtrW( WindowHandle, GWL_EXSTYLE ) );
    RECT BorderRect{ 0, 0, 0, 0 };
    if ( AdjustWindowRectEx( &BorderRect, Style, FALSE, ExStyle ) == FALSE )
    {
        return 0;
    }

    return static_cast<Int32>( -BorderRect.left );
}

Lumen::Int32 Lumen::FWindowsWindow::GetOSWindowTitleBarSize () const noexcept
{
    if ( WindowHandle == nullptr )
    {
        return 0;
    }

    const DWORD Style = static_cast<DWORD>( GetWindowLongPtrW( WindowHandle, GWL_STYLE ) );
    if ( ( Style & WS_CAPTION ) != WS_CAPTION )
    {
        return 0;
    }

    const DWORD ExStyle = static_cast<DWORD>( GetWindowLongPtrW( WindowHandle, GWL_EXSTYLE ) );
    RECT BorderRect{ 0, 0, 0, 0 };
    if ( AdjustWindowRectEx( &BorderRect, Style, FALSE, ExStyle ) == FALSE )
    {
        return 0;
    }

    return static_cast<Int32>( -BorderRect.top + BorderRect.left );
}

Lumen::Bool Lumen::FWindowsWindow::IsForegroundWindow () const noexcept
{
    return ( WindowHandle != nullptr ) and ( GetForegroundWindow() == WindowHandle );
}

void Lumen::FWindowsWindow::SetWindowTitle ( const AnsiChar *const InText ) noexcept
{
    if ( WindowHandle != nullptr and InText != nullptr )
    {
        const std::wstring WideTitle = Utf8ToWide( InText );
        SetWindowTextW( WindowHandle, WideTitle.c_str() );
        if ( Description.IsValid() )
        {
            Description->Title = InText;
        }
    }
}

Lumen::Int32 Lumen::FWindowsWindow::GetWindowWidth () const noexcept
{
    return GetWindowSize().X;
}

Lumen::Int32 Lumen::FWindowsWindow::GetWindowHeight () const noexcept
{
    return GetWindowSize().Y;
}

Lumen::Maths::FVec2i Lumen::FWindowsWindow::GetWindowSize () const noexcept
{
    Maths::FVec2i Size( 0, 0 );
    if ( WindowHandle != nullptr )
    {
        RECT ClientRect{};
        if ( GetClientRect( WindowHandle, &ClientRect ) != FALSE )
        {
            Size.X = static_cast<Int32>( ClientRect.right - ClientRect.left );
            Size.Y = static_cast<Int32>( ClientRect.bottom - ClientRect.top );
        }
    }
    return Size;
}

Lumen::Maths::FVec2i Lumen::FWindowsWindow::GetWindowPosition () const noexcept
{
    Maths::FVec2i Position( 0, 0 );
    if ( WindowHandle != nullptr )
    {
        RECT WindowRect{};
        if ( GetWindowRect( WindowHandle, &WindowRect ) != FALSE )
        {
            Position.X = static_cast<Int32>( WindowRect.left );
            Position.Y = static_cast<Int32>( WindowRect.top );
        }
    }
    return Position;
}

Lumen::TOptional<Lumen::FNativeWindowHandles> Lumen::FWindowsWindow::GetNativeHandles () const noexcept
{
    if ( WindowHandle == nullptr )
    {
        return {};
    }

    const Maths::FVec2i ClientSize = GetWindowSize();
    if ( ClientSize.X <= 0 or ClientSize.Y <= 0 )
    {
        return {};
    }

    const HINSTANCE ResolvedInstance = ( InstanceHandle != nullptr )
                                           ? InstanceHandle
                                           : reinterpret_cast<HINSTANCE>( GetWindowLongPtrW( WindowHandle, GWLP_HINSTANCE ) );

    FNativeWindowHandles Handles;
    Handles.WindowSystem           = ENativeWindowSystem::Win32;
    Handles.NativeWindow           = reinterpret_cast<UPtr>( WindowHandle );
    Handles.NativeDisplay          = reinterpret_cast<UPtr>( ResolvedInstance );
    Handles.NativeWindowAttachment = 0U;
    Handles.Width                  = static_cast<UInt32>( ClientSize.X );
    Handles.Height                 = static_cast<UInt32>( ClientSize.Y );

    return TOptional<FNativeWindowHandles>( Handles );
}

HWND Lumen::FWindowsWindow::GetOSWindowHandle () const noexcept
{
    return WindowHandle;
}

HINSTANCE Lumen::FWindowsWindow::GetOSInstanceHandle () const noexcept
{
    return InstanceHandle;
}

const Lumen::WideChar *Lumen::FWindowsWindow::GetWindowClassName () noexcept
{
    return LumenWindowClassName;
}

#endif
