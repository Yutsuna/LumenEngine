/**
 * @file ICursor.cpp
 * @brief Default Implementation of the ICursor interface for cursor management.
 */

#include "Generic/ICursor.hpp"

void LumenEngine::ICursor::SetPosition ( const Math::FVec2d &Position )
{
    CurrentPosition = Position;
}

const LumenEngine::Math::FVec2d &LumenEngine::ICursor::GetPosition () const
{
    return CurrentPosition;
}

void LumenEngine::ICursor::SetCursorType ( const ECursorType::Type InCursorType )
{
    CurrentCursorType = InCursorType;
}

LumenEngine::ECursorType::Type LumenEngine::ICursor::GetCursorType () const
{
    return CurrentCursorType;
}

LumenEngine::Bool LumenEngine::ICursor::IsCursorVisible () const
{
    return not bHidden;
}
