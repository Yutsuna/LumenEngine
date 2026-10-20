/// @file GenericWindow.cpp
/// @brief Implementation of the FGenericWindow class for cross-platform window management.

#include <Generic/GenericWindow.hpp>

void Lumen::FGenericWindow::Clear () noexcept
{
    /// NOTE: "To be implemented by children
}

void Lumen::FGenericWindow::Destroy () noexcept
{
    /// NOTE: "To be implemented by children
}

void Lumen::FGenericWindow::Show () noexcept
{
    /// NOTE: "To be implemented by children
}

void Lumen::FGenericWindow::Minimize () noexcept
{
    /// NOTE: "To be implemented by children
}

void Lumen::FGenericWindow::Maximize () noexcept
{
    /// NOTE: "To be implemented by children
}

void Lumen::FGenericWindow::Hide () noexcept
{
    /// NOTE: "To be implemented by children
}

void Lumen::FGenericWindow::SetWindowMode ( const EWindowMode::Type LUMEN_UNUSED InWindowMode ) noexcept
{
    /// NOTE: "To be implemented by children
}

Lumen::EWindowMode::Type Lumen::FGenericWindow::GetWindowMode () const noexcept
{
    return EWindowMode::Windowed;
}

Lumen::Bool Lumen::FGenericWindow::IsMaximized () const noexcept
{
    return false;
}

Lumen::Bool Lumen::FGenericWindow::IsMinimized () const noexcept
{
    return false;
}

Lumen::Bool Lumen::FGenericWindow::IsVisible () const noexcept
{
    return Description->bIsVisible;
}

void Lumen::FGenericWindow::SetWindowFocus () noexcept
{
    /// NOTE: "To be implemented by children
}

void Lumen::FGenericWindow::SetOpacity ( const Float32 LUMEN_UNUSED InOpacity ) noexcept
{
    /// NOTE: "To be implemented by children
}

void Lumen::FGenericWindow::EnableInput ( Bool LUMEN_UNUSED bEnable ) noexcept
{
    /// NOTE: "To be implemented by children
}

Lumen::Bool Lumen::FGenericWindow::IsPointInWindow ( const Int32 InX, const Int32 InY ) const noexcept
{
    const Maths::FVec2i &WindowPosition = GetWindowPosition ();
    const Maths::FVec2i &WindowSize = GetWindowSize ();

    const Int32 Left = WindowPosition.X;
    const Int32 Top = WindowPosition.Y;
    const Int32 Right = WindowPosition.X + WindowSize.X;
    const Int32 Bottom = WindowPosition.Y + WindowSize.Y;

    const Bool bIsInsideX = ( InX >= Left ) and ( InX < Right );
    const Bool bIsInsideY = ( InY >= Top ) and ( InY < Bottom );

    return bIsInsideX and bIsInsideY;
}

Lumen::Int32 Lumen::FGenericWindow::GetOSWindowBorderSize () const noexcept
{
    return 0;
}

Lumen::Int32 Lumen::FGenericWindow::GetOSWindowTitleBarSize () const noexcept
{
    return 0;
}

Lumen::Bool Lumen::FGenericWindow::IsForegroundWindow () const noexcept
{
    return false;
}

void Lumen::FGenericWindow::SetWindowTitle ( const AnsiChar LUMEN_UNUSED *const InText ) noexcept
{
    /// NOTE: "To be implemented by children
}

Lumen::Int32 Lumen::FGenericWindow::GetWindowWidth () const noexcept
{
    return Description->Size.Width;
}

Lumen::Int32 Lumen::FGenericWindow::GetWindowHeight () const noexcept
{
    return Description->Size.Height;
}

Lumen::Maths::FVec2i Lumen::FGenericWindow::GetWindowSize () const noexcept
{
    return Description->Size;
}

Lumen::Maths::FVec2i Lumen::FGenericWindow::GetWindowPosition () const noexcept
{
    return Description->Position;
}
