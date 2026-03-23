/**
 * @file GenericWindow.cpp
 * @brief Implementation of the FGenericWindow class for cross-platform window management.
 */

#include "Generic/GenericWindow.hpp"

void LumenEngine::FGenericWindow::Reshape ( const Int32, const Int32, const Int32, const Int32 )
{
    /* Empty */
}

void LumenEngine::FGenericWindow::Destroy ()
{
    /* Empty */
}

void LumenEngine::FGenericWindow::Minimize ()
{
    /* Empty */
}

void LumenEngine::FGenericWindow::Maximize ()
{
    /* Empty */
}

void LumenEngine::FGenericWindow::Restore ()
{
    /* Empty */
}

void LumenEngine::FGenericWindow::Show ()
{
    /* Empty */
}

void LumenEngine::FGenericWindow::Hide ()
{
    /* Empty */
}

void LumenEngine::FGenericWindow::Clear ()
{
    /* Empty */
}

void LumenEngine::FGenericWindow::SetWindowMode ( const EWindowMode::Type )
{
    /* Empty */
}

LumenEngine::EWindowMode::Type LumenEngine::FGenericWindow::GetWindowMode () const
{
    return EWindowMode::Windowed;
}

LumenEngine::Bool LumenEngine::FGenericWindow::IsMaximized () const
{
    return false;
}

LumenEngine::Bool LumenEngine::FGenericWindow::IsMinimized () const
{
    return false;
}

LumenEngine::Bool LumenEngine::FGenericWindow::IsVisible () const
{
    return Description->bIsVisible;
}

void LumenEngine::FGenericWindow::SetWindowFocus ()
{
    /* Empty */
}

void LumenEngine::FGenericWindow::SetOpacity ( const Float32 )
{
    /* Empty */
}

void LumenEngine::FGenericWindow::EnableInput ( const Bool )
{
    /* Empty */
}

LumenEngine::Bool LumenEngine::FGenericWindow::IsPointInWindow ( const Int32 InX, const Int32 InY ) const
{
    const Maths::FVec2i &WindowPos  = GetWindowPosition();
    const Maths::FVec2i &WindowSize = GetWindowSize();

    const Int32 Left   = WindowPos.X;
    const Int32 Right  = WindowPos.X + WindowSize.Width;
    const Int32 Top    = WindowPos.Y;
    const Int32 Bottom = WindowPos.Y + WindowSize.Height;

    const Bool bIsInsideX = ( InX >= Left ) && ( InX < Right );
    const Bool bIsInsideY = ( InY >= Top ) && ( InY < Bottom );

    return bIsInsideX && bIsInsideY;
}

LumenEngine::Int32 LumenEngine::FGenericWindow::GetOSWindowBorderSize () const
{
    return 0;
}

LumenEngine::Int32 LumenEngine::FGenericWindow::GetOSWindowTitleBarSize () const
{
    return 0;
}

LumenEngine::Bool LumenEngine::FGenericWindow::IsForegroundWindow () const
{
    return false;
}

void LumenEngine::FGenericWindow::SetWindowTitle ( const AnsiChar *const )
{
    /* Empty */
}

LumenEngine::Int32 LumenEngine::FGenericWindow::GetWindowWidth () const
{
    return Description->Size.Width;
}

LumenEngine::Int32 LumenEngine::FGenericWindow::GetWindowHeight () const
{
    return Description->Size.Height;
}

const LumenEngine::Maths::FVec2i &LumenEngine::FGenericWindow::GetWindowSize () const
{
    return Description->Size;
}

const LumenEngine::Maths::FVec2i &LumenEngine::FGenericWindow::GetWindowPosition () const
{
    return Description->Position;
}
