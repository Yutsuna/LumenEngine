///
/// @file GenericApplicationMessageHandler.cpp
/// @brief Default Implementation of the FGenericApplicationMessageHandler class for handling application messages.
///

#include <Core/CoreDefinitions.hpp>
#include <Generic/GenericApplicationMessageHandler.hpp>

/// Application

void Lumen::FGenericApplicationMessageHandler::OnRequestExit () noexcept
{
    /// Empty
}

void Lumen::FGenericApplicationMessageHandler::OnQuit () noexcept
{
    /// Empty
}

/// Window

void Lumen::FGenericApplicationMessageHandler::OnOSPaint ( const TSharedRef<FGenericWindow> &LUMEN_UNUSED InWindow ) noexcept
{
    /// Empty
}

void Lumen::FGenericApplicationMessageHandler::OnWindowMoved ( const TSharedRef<FGenericWindow> &LUMEN_UNUSED InWindow,
                                                               Int32 LUMEN_UNUSED InX,
                                                               Int32 LUMEN_UNUSED InY ) noexcept
{
    /// Empty
}

void Lumen::FGenericApplicationMessageHandler::OnWindowResized ( const TSharedRef<FGenericWindow> &LUMEN_UNUSED InWindow,
                                                                 Int32 LUMEN_UNUSED InX,
                                                                 Int32 LUMEN_UNUSED InY ) noexcept
{
    /// Empty
}

void Lumen::FGenericApplicationMessageHandler::OnWindowActivationChanged (  const TSharedRef<FGenericWindow> &LUMEN_UNUSED InWindow ) noexcept,
                                                                            Bool LUMEN_UNUSED bIsRepeated ) noexcept
{
    /// Empty
}

/// Mouse

void Lumen::FGenericApplicationMessageHandler::OnMouseDown ( EMouseButton::Type LUMEN_UNUSED InMouseButton ) noexcept
{
    /// Empty
}

void Lumen::FGenericApplicationMessageHandler::OnMouseUp ( EMouseButton::Type LUMEN_UNUSED InMouseButton ) noexcept
{
    /// Empty
}

void Lumen::FGenericApplicationMessageHandler::OnMouseMove ( Int32 LUMEN_UNUSED InX, Int32 LUMEN_UNUSED IntY ) noexcept
{
    /// Empty
}

void Lumen::FGenericApplicationMessageHandler::OnMouseWheel ( Int32 LUMEN_UNUSED InDelta ) noexcept
{
    /// Empty
}

/// Keyboard

void Lumen::FGenericApplicationMessageHandler::OnKeyDown ( EKeys::Type LUMEN_UNUSED InKey,
                                                           Bool LUMEN_UNUSED bIsRepeated ) noexcept
{
    /// Empty
}

void Lumen::FGenericApplicationMessageHandler::OnKeyUp ( EKeys::Type LUMEN_UNUSED InKey, Bool LUMEN_UNUSED bIsRepeated ) noexcept
{
    /// Empty
}
