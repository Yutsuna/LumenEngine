/**
 * @file GenericWindow.cpp
 * @brief Implementation of the FGenericWindow class for cross-platform window management.
 */

#include "Generic/GenericWindow.hpp"

void LumenEngine::FGenericWindow::Reshape ( const Int32 InX,
                                            const Int32 InY,
                                            const Int32 InWidth,
                                            const Int32 InHeight )
{
    /* __empty__ */
}

void LumenEngine::FGenericWindow::Destroy ()
{
    /* __empty__ */
}

void LumenEngine::FGenericWindow::Minimize ()
{
    /* __empty__ */
}

void LumenEngine::FGenericWindow::Maximize ()
{
    /* __empty__ */
}

void LumenEngine::FGenericWindow::Restore ()
{
    /* __empty__ */
}

void LumenEngine::FGenericWindow::Show ()
{
    /* __empty__ */
}

void LumenEngine::FGenericWindow::Hide ()
{
    /* __empty__ */
}

void LumenEngine::FGenericWindow::SetWindowMode ( const EWindowMode::Type InWindowMode )
{
    /* __empty__ */
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
    /* __empty__ */
}

void LumenEngine::FGenericWindow::SetOpacity ( const Float32 InOpacity )
{
    /* __empty__ */
}

void LumenEngine::FGenericWindow::EnableInput ( const Bool bEnable )
{
    /* __empty__ */
}

LumenEngine::Bool LumenEngine::FGenericWindow::IsPointInWindow ( const Int32 InX, const Int32 InY ) const
{
    const Math::FVec2i &WindowPos  = GetWindowPosition();
    const Math::FVec2i &WindowSize = GetWindowSize();

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

void LumenEngine::FGenericWindow::SetWindowTitle ( const AnsiChar *const Text )
{
    /* __empty__ */
}

LumenEngine::Int32 LumenEngine::FGenericWindow::GetWindowWidth () const
{
    return Description->Size.Width;
}

LumenEngine::Int32 LumenEngine::FGenericWindow::GetWindowHeight () const
{
    return Description->Size.Height;
}

const LumenEngine::Math::FVec2i &LumenEngine::FGenericWindow::GetWindowSize () const
{
    return Description->Size;
}

const LumenEngine::Math::FVec2i &LumenEngine::FGenericWindow::GetWindowPosition () const
{
    return Description->Position;
}
