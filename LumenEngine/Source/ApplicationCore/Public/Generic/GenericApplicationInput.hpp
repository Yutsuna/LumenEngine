/**
 * @file GenericApplicationInput.hpp
 * @brief Declaration of the FGenericApplicationInput struct for generic application input handling.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

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

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )

    #include <SDL3/SDL.h>

static inline LumenEngine::EKeys::Type TranslateSDLKeyCodeToEKeys ( const SDL_Keycode KeyCode )
{
    switch ( KeyCode )
    {
    case SDLK_UNKNOWN:
        break;
    case SDLK_RETURN:
        break;
    case SDLK_ESCAPE:
        break;
    case SDLK_BACKSPACE:
        break;
    case SDLK_TAB:
        break;
    case SDLK_SPACE:
        break;
    case SDLK_EXCLAIM:
        break;
    case SDLK_DBLAPOSTROPHE:
        break;
    case SDLK_HASH:
        break;
    case SDLK_DOLLAR:
        break;
    case SDLK_PERCENT:
        break;
    case SDLK_AMPERSAND:
        break;
    case SDLK_APOSTROPHE:
        break;
    case SDLK_LEFTPAREN:
        break;
    case SDLK_RIGHTPAREN:
        break;
    case SDLK_ASTERISK:
        break;
    case SDLK_PLUS:
        break;
    case SDLK_COMMA:
        break;
    case SDLK_MINUS:
        break;
    case SDLK_PERIOD:
        break;
    case SDLK_SLASH:
        break;
    case SDLK_0:
        break;
    case SDLK_1:
        break;
    case SDLK_2:
        break;
    case SDLK_3:
        break;
    case SDLK_4:
        break;
    case SDLK_5:
        break;
    case SDLK_6:
        break;
    case SDLK_7:
        break;
    case SDLK_8:
        break;
    case SDLK_9:
        break;
    case SDLK_COLON:
        break;
    case SDLK_SEMICOLON:
        break;
    case SDLK_LESS:
        break;
    case SDLK_EQUALS:
        break;
    case SDLK_GREATER:
        break;
    case SDLK_QUESTION:
        break;
    case SDLK_AT:
        break;
    case SDLK_LEFTBRACKET:
        break;
    case SDLK_BACKSLASH:
        break;
    case SDLK_RIGHTBRACKET:
        break;
    case SDLK_CARET:
        break;
    case SDLK_UNDERSCORE:
        break;
    case SDLK_GRAVE:
        break;
    case SDLK_A:
        break;
    case SDLK_B:
        break;
    case SDLK_C:
        break;
    case SDLK_D:
        break;
    case SDLK_E:
        break;
    case SDLK_F:
        break;
    case SDLK_G:
        break;
    case SDLK_H:
        break;
    case SDLK_I:
        break;
    case SDLK_J:
        break;
    case SDLK_K:
        break;
    case SDLK_L:
        break;
    case SDLK_M:
        break;
    case SDLK_N:
        break;
    case SDLK_O:
        break;
    case SDLK_P:
        break;
    case SDLK_S:
        break;
    case SDLK_T:
        break;
    case SDLK_U:
        break;
    case SDLK_V:
        break;
    case SDLK_W:
        break;
    case SDLK_X:
        break;
    case SDLK_Y:
        break;
    case SDLK_Z:
        break;
    case SDLK_LEFTBRACE:
        break;
    case SDLK_PIPE:
        break;
    case SDLK_RIGHTBRACE:
        break;
    case SDLK_TILDE:
        break;
    case SDLK_DELETE:
        break;
    case SDLK_PLUSMINUS:
        break;
    case SDLK_CAPSLOCK:
        break;
    case SDLK_F1:
        break;
    case SDLK_F2:
        break;
    case SDLK_F3:
        break;
    case SDLK_F4:
        break;
    case SDLK_F5:
        break;
    case SDLK_F6:
        break;
    case SDLK_F7:
        break;
    case SDLK_F8:
        break;
    case SDLK_F9:
        break;
    case SDLK_F10:
        break;
    case SDLK_F11:
        break;
    case SDLK_F12:
        break;
    case SDLK_PRINTSCREEN:
        break;
    case SDLK_SCROLLLOCK:
        break;
    case SDLK_PAUSE:
        break;
    case SDLK_INSERT:
        break;
    case SDLK_HOME:
        break;
    case SDLK_PAGEUP:
        break;
    case SDLK_END:
        break;
    case SDLK_PAGEDOWN:
        break;
    case SDLK_RIGHT:
        break;
    case SDLK_LEFT:
        break;
    case SDLK_DOWN:
        break;
    case SDLK_UP:
        break;
    case SDLK_NUMLOCKCLEAR:
        break;
    case SDLK_KP_DIVIDE:
        break;
    case SDLK_KP_MULTIPLY:
        break;
    case SDLK_KP_MINUS:
        break;
    case SDLK_KP_PLUS:
        break;
    case SDLK_KP_ENTER:
        break;
    case SDLK_KP_1:
        break;
    case SDLK_KP_2:
        break;
    case SDLK_KP_3:
        break;
    case SDLK_KP_4:
        break;
    case SDLK_KP_5:
        break;
    case SDLK_KP_6:
        break;
    case SDLK_KP_7:
        break;
    case SDLK_KP_8:
        break;
    case SDLK_KP_9:
        break;
    case SDLK_KP_0:
        break;
    case SDLK_KP_PERIOD:
        break;
    case SDLK_APPLICATION:
        break;
    case SDLK_POWER:
        break;
    case SDLK_KP_EQUALS:
        break;
    case SDLK_F13:
        break;
    case SDLK_F14:
        break;
    case SDLK_F15:
        break;
    case SDLK_F16:
        break;
    case SDLK_F17:
        break;
    case SDLK_F18:
        break;
    case SDLK_F19:
        break;
    case SDLK_F20:
        break;
    case SDLK_F21:
        break;
    case SDLK_F22:
        break;
    case SDLK_F23:
        break;
    case SDLK_F24:
        break;
    case SDLK_EXECUTE:
        break;
    case SDLK_HELP:
        break;
    case SDLK_MENU:
        break;
    case SDLK_SELECT:
        break;
    case SDLK_STOP:
        break;
    case SDLK_AGAIN:
        break;
    case SDLK_UNDO:
        break;
    case SDLK_CUT:
        break;
    case SDLK_COPY:
        break;
    case SDLK_PASTE:
        break;
    case SDLK_FIND:
        break;
    case SDLK_MUTE:
        break;
    case SDLK_VOLUMEUP:
        break;
    case SDLK_VOLUMEDOWN:
        break;
    case SDLK_KP_COMMA:
        break;
    case SDLK_KP_EQUALSAS400:
        break;
    case SDLK_ALTERASE:
        break;
    case SDLK_SYSREQ:
        break;
    case SDLK_CANCEL:
        break;
    case SDLK_CLEAR:
        break;
    case SDLK_PRIOR:
        break;
    case SDLK_RETURN2:
        break;
    case SDLK_SEPARATOR:
        break;
    case SDLK_OUT:
        break;
    case SDLK_OPER:
        break;
    case SDLK_CLEARAGAIN:
        break;
    case SDLK_CRSEL:
        break;
    case SDLK_EXSEL:
        break;
    case SDLK_KP_00:
        break;
    case SDLK_KP_000:
        break;
    case SDLK_THOUSANDSSEPARATOR:
        break;
    case SDLK_DECIMALSEPARATOR:
        break;
    case SDLK_CURRENCYUNIT:
        break;
    case SDLK_CURRENCYSUBUNIT:
        break;
    case SDLK_KP_LEFTPAREN:
        break;
    case SDLK_KP_RIGHTPAREN:
        break;
    case SDLK_KP_LEFTBRACE:
        break;
    case SDLK_KP_RIGHTBRACE:
        break;
    case SDLK_KP_TAB:
        break;
    case SDLK_KP_BACKSPACE:
        break;
    case SDLK_KP_A:
        break;
    case SDLK_KP_B:
        break;
    case SDLK_KP_C:
        break;
    case SDLK_KP_D:
        break;
    case SDLK_KP_E:
        break;
    case SDLK_KP_F:
        break;
    case SDLK_KP_XOR:
        break;
    case SDLK_KP_POWER:
        break;
    case SDLK_KP_PERCENT:
        break;
    case SDLK_KP_LESS:
        break;
    case SDLK_KP_GREATER:
        break;
    case SDLK_KP_AMPERSAND:
        break;
    case SDLK_KP_DBLAMPERSAND:
        break;
    case SDLK_KP_VERTICALBAR:
        break;
    case SDLK_KP_DBLVERTICALBAR:
        break;
    case SDLK_KP_COLON:
        break;
    case SDLK_KP_HASH:
        break;
    case SDLK_KP_SPACE:
        break;
    case SDLK_KP_AT:
        break;
    case SDLK_KP_EXCLAM:
        break;
    case SDLK_KP_MEMSTORE:
        break;
    case SDLK_KP_MEMRECALL:
        break;
    case SDLK_KP_MEMCLEAR:
        break;
    case SDLK_KP_MEMADD:
        break;
    case SDLK_KP_MEMSUBTRACT:
        break;
    case SDLK_KP_MEMMULTIPLY:
        break;
    case SDLK_KP_MEMDIVIDE:
        break;
    case SDLK_KP_PLUSMINUS:
        break;
    case SDLK_KP_CLEAR:
        break;
    case SDLK_KP_CLEARENTRY:
        break;
    case SDLK_KP_BINARY:
        break;
    case SDLK_KP_OCTAL:
        break;
    case SDLK_KP_DECIMAL:
        break;
    case SDLK_KP_HEXADECIMAL:
        break;
    case SDLK_LCTRL:
        break;
    case SDLK_LSHIFT:
        break;
    case SDLK_LALT:
        break;
    case SDLK_LGUI:
        break;
    case SDLK_RCTRL:
        break;
    case SDLK_RSHIFT:
        break;
    case SDLK_RALT:
        break;
    case SDLK_RGUI:
        break;
    case SDLK_MODE:
        break;
    case SDLK_SLEEP:
        break;
    case SDLK_WAKE:
        break;
    case SDLK_CHANNEL_INCREMENT:
        break;
    case SDLK_CHANNEL_DECREMENT:
        break;
    case SDLK_MEDIA_PLAY:
        break;
    case SDLK_MEDIA_PAUSE:
        break;
    case SDLK_MEDIA_RECORD:
        break;
    case SDLK_MEDIA_FAST_FORWARD:
        break;
    case SDLK_MEDIA_REWIND:
        break;
    case SDLK_MEDIA_NEXT_TRACK:
        break;
    case SDLK_MEDIA_PREVIOUS_TRACK:
        break;
    case SDLK_MEDIA_STOP:
        break;
    case SDLK_MEDIA_EJECT:
        break;
    case SDLK_MEDIA_PLAY_PAUSE:
        break;
    case SDLK_MEDIA_SELECT:
        break;
    case SDLK_AC_NEW:
        break;
    case SDLK_AC_OPEN:
        break;
    case SDLK_AC_CLOSE:
        break;
    case SDLK_AC_EXIT:
        break;
    case SDLK_AC_SAVE:
        break;
    case SDLK_AC_PRINT:
        break;
    case SDLK_AC_PROPERTIES:
        break;
    case SDLK_AC_SEARCH:
        break;
    case SDLK_AC_HOME:
        break;
    case SDLK_AC_BACK:
        break;
    case SDLK_AC_FORWARD:
        break;
    case SDLK_AC_STOP:
        break;
    case SDLK_AC_REFRESH:
        break;
    case SDLK_AC_BOOKMARKS:
        break;
    case SDLK_SOFTLEFT:
        break;
    case SDLK_SOFTRIGHT:
        break;
    case SDLK_CALL:
        break;
    case SDLK_ENDCALL:
        break;
    case SDLK_LEFT_TAB:
        break;
    case SDLK_LEVEL5_SHIFT:
        break;
    case SDLK_MULTI_KEY_COMPOSE:
        break;
    case SDLK_LMETA:
        break;
    case SDLK_RMETA:
        break;
    case SDLK_LHYPER:
        break;
    case SDLK_RHYPER:
        break;
    default:
        return LumenEngine::EKeys::Unknown;
    }
    return LumenEngine::EKeys::Unknown;
}

#endif
