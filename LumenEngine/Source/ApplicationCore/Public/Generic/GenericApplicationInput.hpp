/**
 * @file GenericApplicationInput.hpp
 * @brief Declaration of the FGenericApplicationInput struct for generic application input handling.
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace EMouseButton
{

    enum Type : UInt8
    {
        Left    = 0,
        Middle  = 1,
        Right   = 2,
        Button4 = 3,
        Button5 = 4,
        Invalid = 255
    };

} // namespace EMouseButton

namespace EModifierKey
{

    enum Type : UInt32
    {
        None    = 0,
        Shift   = 1 << 0,
        Control = 1 << 1,
        Alt     = 1 << 2,
        Command = 1 << 3,
    };

} // namespace EModifierKey

namespace EKeys
{

    enum Type : UInt32
    {
        Unknown = 0,
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        Num0,
        Num1,
        Num2,
        Num3,
        Num4,
        Num5,
        Num6,
        Num7,
        Num8,
        Num9,
        Escape,
        Enter,
        Spacebar,
        Backspace,
        Tab,
        Shift,
        Control,
        Alt,
        Up,
        Down,
        Left,
        Right,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12
    };

}

} // namespace LumenEngine
