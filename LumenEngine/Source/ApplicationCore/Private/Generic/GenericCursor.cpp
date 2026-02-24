/**
 * @file GenericCursor.cpp
 * @brief Default Implementation of the FGenericCursor interface for cursor management.
 */

#include "Generic/GenericCursor.hpp"

void LumenEngine::FGenericCursor::SetPosition ( const Math::FVec2d &Position )
{
    CurrentPosition = Position;
}

const LumenEngine::Math::FVec2d &LumenEngine::FGenericCursor::GetPosition () const
{
    return CurrentPosition;
}

void LumenEngine::FGenericCursor::SetCursorType ( const ECursorType::Type InCursorType )
{
    CurrentCursorType = InCursorType;
}

LumenEngine::ECursorType::Type LumenEngine::FGenericCursor::GetCursorType () const
{
    return CurrentCursorType;
}

LumenEngine::Bool LumenEngine::FGenericCursor::IsCursorVisible () const
{
    return not bHidden;
}
