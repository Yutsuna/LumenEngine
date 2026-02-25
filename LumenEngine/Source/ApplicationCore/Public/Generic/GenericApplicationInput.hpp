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
        F12,

        /** Punctuation & Symbols */
        Exclamation,
        DoubleApostrophe,
        Hash,
        Dollar,
        Percent,
        Ampersand,
        Apostrophe,
        LeftParenthesis,
        RightParenthesis,
        Asterisk,
        Plus,
        Comma,
        Minus,
        Period,
        Slash,
        Colon,
        Semicolon,
        Less,
        Equals,
        Greater,
        Question,
        At,
        LeftBracket,
        Backslash,
        RightBracket,
        Caret,
        Underscore,
        GraveAccent,
        LeftBrace,
        Pipe,
        RightBrace,
        Tilde,

        /** Editing & Navigation */
        Delete,
        PlusMinus,
        CapsLock,
        PrintScreen,
        ScrollLock,
        Pause,
        Insert,
        Home,
        PageUp,
        End,
        PageDown,
        NumLock,

        /** Numpad */
        NumPadDivide,
        NumPadMultiply,
        NumPadMinus,
        NumPadPlus,
        NumPadEnter,
        NumPad1,
        NumPad2,
        NumPad3,
        NumPad4,
        NumPad5,
        NumPad6,
        NumPad7,
        NumPad8,
        NumPad9,
        NumPad0,
        NumPadPeriod,
        NumPadEquals,
        NumPadComma,
        NumPadEqualsAs400,
        NumPad00,
        NumPad000,
        NumPadLeftParenthesis,
        NumPadRightParenthesis,
        NumPadLeftBrace,
        NumPadRightBrace,
        NumPadTab,
        NumPadBackspace,
        NumPadA,
        NumPadB,
        NumPadC,
        NumPadD,
        NumPadE,
        NumPadF,
        NumPadXor,
        NumPadPower,
        NumPadPercent,
        NumPadLess,
        NumPadGreater,
        NumPadAmpersand,
        NumPadDoubleAmpersand,
        NumPadVerticalBar,
        NumPadDoubleVerticalBar,
        NumPadColon,
        NumPadHash,
        NumPadSpace,
        NumPadAt,
        NumPadExclamation,
        NumPadMemStore,
        NumPadMemRecall,
        NumPadMemClear,
        NumPadMemAdd,
        NumPadMemSubtract,
        NumPadMemMultiply,
        NumPadMemDivide,
        NumPadPlusMinus,
        NumPadClear,
        NumPadClearEntry,
        NumPadBinary,
        NumPadOctal,
        NumPadDecimal,
        NumPadHexadecimal,

        /** Function Keys 13-24 */
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24,

        /** Modifiers */
        LeftControl,
        LeftShift,
        LeftAlt,
        LeftGui,
        RightControl,
        RightShift,
        RightAlt,
        RightGui,
        LeftMeta,
        RightMeta,
        LeftHyper,
        RightHyper,
        Level5Shift,

        /** Application / Multimedia / System */
        Application,
        Power,
        Execute,
        Help,
        Menu,
        Select,
        Stop,
        Again,
        Undo,
        Cut,
        Copy,
        Paste,
        Find,
        Mute,
        VolumeUp,
        VolumeDown,
        AltErase,
        SysReq,
        Cancel,
        Clear,
        Prior,
        Return2,
        Separator,
        Out,
        Oper,
        ClearAgain,
        CrSel,
        ExSel,
        ThousandsSeparator,
        DecimalSeparator,
        CurrencyUnit,
        CurrencySubUnit,

        Mode,
        Sleep,
        Wake,
        ChannelIncrement,
        ChannelDecrement,
        MediaPlay,
        MediaPause,
        MediaRecord,
        MediaFastForward,
        MediaRewind,
        MediaNextTrack,
        MediaPreviousTrack,
        MediaStop,
        MediaEject,
        MediaPlayPause,
        MediaSelect,

        AcNew,
        AcOpen,
        AcClose,
        AcExit,
        AcSave,
        AcPrint,
        AcProperties,
        AcSearch,
        AcHome,
        AcBack,
        AcForward,
        AcStop,
        AcRefresh,
        AcBookmarks,

        SoftLeft,
        SoftRight,
        Call,
        EndCall,
        LeftTab,
        MultiKeyCompose
    };

} // namespace EKeys

} // namespace LumenEngine

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )

    #include <SDL3/SDL.h>

static inline LumenEngine::EKeys::Type TranslateSDLKeyCodeToEKeys ( const SDL_Keycode KeyCode )
{
    switch ( KeyCode )
    {
    case SDLK_UNKNOWN:
        return LumenEngine::EKeys::Unknown;
    case SDLK_RETURN:
        return LumenEngine::EKeys::Enter;
    case SDLK_ESCAPE:
        return LumenEngine::EKeys::Escape;
    case SDLK_BACKSPACE:
        return LumenEngine::EKeys::Backspace;
    case SDLK_TAB:
        return LumenEngine::EKeys::Tab;
    case SDLK_SPACE:
        return LumenEngine::EKeys::Spacebar;
    case SDLK_EXCLAIM:
        return LumenEngine::EKeys::Exclamation;
    case SDLK_DBLAPOSTROPHE:
        return LumenEngine::EKeys::DoubleApostrophe;
    case SDLK_HASH:
        return LumenEngine::EKeys::Hash;
    case SDLK_DOLLAR:
        return LumenEngine::EKeys::Dollar;
    case SDLK_PERCENT:
        return LumenEngine::EKeys::Percent;
    case SDLK_AMPERSAND:
        return LumenEngine::EKeys::Ampersand;
    case SDLK_APOSTROPHE:
        return LumenEngine::EKeys::Apostrophe;
    case SDLK_LEFTPAREN:
        return LumenEngine::EKeys::LeftParenthesis;
    case SDLK_RIGHTPAREN:
        return LumenEngine::EKeys::RightParenthesis;
    case SDLK_ASTERISK:
        return LumenEngine::EKeys::Asterisk;
    case SDLK_PLUS:
        return LumenEngine::EKeys::Plus;
    case SDLK_COMMA:
        return LumenEngine::EKeys::Comma;
    case SDLK_MINUS:
        return LumenEngine::EKeys::Minus;
    case SDLK_PERIOD:
        return LumenEngine::EKeys::Period;
    case SDLK_SLASH:
        return LumenEngine::EKeys::Slash;
    case SDLK_0:
        return LumenEngine::EKeys::Num0;
    case SDLK_1:
        return LumenEngine::EKeys::Num1;
    case SDLK_2:
        return LumenEngine::EKeys::Num2;
    case SDLK_3:
        return LumenEngine::EKeys::Num3;
    case SDLK_4:
        return LumenEngine::EKeys::Num4;
    case SDLK_5:
        return LumenEngine::EKeys::Num5;
    case SDLK_6:
        return LumenEngine::EKeys::Num6;
    case SDLK_7:
        return LumenEngine::EKeys::Num7;
    case SDLK_8:
        return LumenEngine::EKeys::Num8;
    case SDLK_9:
        return LumenEngine::EKeys::Num9;
    case SDLK_COLON:
        return LumenEngine::EKeys::Colon;
    case SDLK_SEMICOLON:
        return LumenEngine::EKeys::Semicolon;
    case SDLK_LESS:
        return LumenEngine::EKeys::Less;
    case SDLK_EQUALS:
        return LumenEngine::EKeys::Equals;
    case SDLK_GREATER:
        return LumenEngine::EKeys::Greater;
    case SDLK_QUESTION:
        return LumenEngine::EKeys::Question;
    case SDLK_AT:
        return LumenEngine::EKeys::At;
    case SDLK_LEFTBRACKET:
        return LumenEngine::EKeys::LeftBracket;
    case SDLK_BACKSLASH:
        return LumenEngine::EKeys::Backslash;
    case SDLK_RIGHTBRACKET:
        return LumenEngine::EKeys::RightBracket;
    case SDLK_CARET:
        return LumenEngine::EKeys::Caret;
    case SDLK_UNDERSCORE:
        return LumenEngine::EKeys::Underscore;
    case SDLK_GRAVE:
        return LumenEngine::EKeys::GraveAccent;
    case SDLK_A:
        return LumenEngine::EKeys::A;
    case SDLK_B:
        return LumenEngine::EKeys::B;
    case SDLK_C:
        return LumenEngine::EKeys::C;
    case SDLK_D:
        return LumenEngine::EKeys::D;
    case SDLK_E:
        return LumenEngine::EKeys::E;
    case SDLK_F:
        return LumenEngine::EKeys::F;
    case SDLK_G:
        return LumenEngine::EKeys::G;
    case SDLK_H:
        return LumenEngine::EKeys::H;
    case SDLK_I:
        return LumenEngine::EKeys::I;
    case SDLK_J:
        return LumenEngine::EKeys::J;
    case SDLK_K:
        return LumenEngine::EKeys::K;
    case SDLK_L:
        return LumenEngine::EKeys::L;
    case SDLK_M:
        return LumenEngine::EKeys::M;
    case SDLK_N:
        return LumenEngine::EKeys::N;
    case SDLK_O:
        return LumenEngine::EKeys::O;
    case SDLK_P:
        return LumenEngine::EKeys::P;
    case SDLK_Q:
        return LumenEngine::EKeys::Q;
    case SDLK_R:
        return LumenEngine::EKeys::R;
    case SDLK_S:
        return LumenEngine::EKeys::S;
    case SDLK_T:
        return LumenEngine::EKeys::T;
    case SDLK_U:
        return LumenEngine::EKeys::U;
    case SDLK_V:
        return LumenEngine::EKeys::V;
    case SDLK_W:
        return LumenEngine::EKeys::W;
    case SDLK_X:
        return LumenEngine::EKeys::X;
    case SDLK_Y:
        return LumenEngine::EKeys::Y;
    case SDLK_Z:
        return LumenEngine::EKeys::Z;
    case SDLK_LEFTBRACE:
        return LumenEngine::EKeys::LeftBrace;
    case SDLK_PIPE:
        return LumenEngine::EKeys::Pipe;
    case SDLK_RIGHTBRACE:
        return LumenEngine::EKeys::RightBrace;
    case SDLK_TILDE:
        return LumenEngine::EKeys::Tilde;
    case SDLK_DELETE:
        return LumenEngine::EKeys::Delete;
    case SDLK_PLUSMINUS:
        return LumenEngine::EKeys::PlusMinus;
    case SDLK_CAPSLOCK:
        return LumenEngine::EKeys::CapsLock;
    case SDLK_F1:
        return LumenEngine::EKeys::F1;
    case SDLK_F2:
        return LumenEngine::EKeys::F2;
    case SDLK_F3:
        return LumenEngine::EKeys::F3;
    case SDLK_F4:
        return LumenEngine::EKeys::F4;
    case SDLK_F5:
        return LumenEngine::EKeys::F5;
    case SDLK_F6:
        return LumenEngine::EKeys::F6;
    case SDLK_F7:
        return LumenEngine::EKeys::F7;
    case SDLK_F8:
        return LumenEngine::EKeys::F8;
    case SDLK_F9:
        return LumenEngine::EKeys::F9;
    case SDLK_F10:
        return LumenEngine::EKeys::F10;
    case SDLK_F11:
        return LumenEngine::EKeys::F11;
    case SDLK_F12:
        return LumenEngine::EKeys::F12;
    case SDLK_PRINTSCREEN:
        return LumenEngine::EKeys::PrintScreen;
    case SDLK_SCROLLLOCK:
        return LumenEngine::EKeys::ScrollLock;
    case SDLK_PAUSE:
        return LumenEngine::EKeys::Pause;
    case SDLK_INSERT:
        return LumenEngine::EKeys::Insert;
    case SDLK_HOME:
        return LumenEngine::EKeys::Home;
    case SDLK_PAGEUP:
        return LumenEngine::EKeys::PageUp;
    case SDLK_END:
        return LumenEngine::EKeys::End;
    case SDLK_PAGEDOWN:
        return LumenEngine::EKeys::PageDown;
    case SDLK_RIGHT:
        return LumenEngine::EKeys::Right;
    case SDLK_LEFT:
        return LumenEngine::EKeys::Left;
    case SDLK_DOWN:
        return LumenEngine::EKeys::Down;
    case SDLK_UP:
        return LumenEngine::EKeys::Up;
    case SDLK_NUMLOCKCLEAR:
        return LumenEngine::EKeys::NumLock;
    case SDLK_KP_DIVIDE:
        return LumenEngine::EKeys::NumPadDivide;
    case SDLK_KP_MULTIPLY:
        return LumenEngine::EKeys::NumPadMultiply;
    case SDLK_KP_MINUS:
        return LumenEngine::EKeys::NumPadMinus;
    case SDLK_KP_PLUS:
        return LumenEngine::EKeys::NumPadPlus;
    case SDLK_KP_ENTER:
        return LumenEngine::EKeys::NumPadEnter;
    case SDLK_KP_1:
        return LumenEngine::EKeys::NumPad1;
    case SDLK_KP_2:
        return LumenEngine::EKeys::NumPad2;
    case SDLK_KP_3:
        return LumenEngine::EKeys::NumPad3;
    case SDLK_KP_4:
        return LumenEngine::EKeys::NumPad4;
    case SDLK_KP_5:
        return LumenEngine::EKeys::NumPad5;
    case SDLK_KP_6:
        return LumenEngine::EKeys::NumPad6;
    case SDLK_KP_7:
        return LumenEngine::EKeys::NumPad7;
    case SDLK_KP_8:
        return LumenEngine::EKeys::NumPad8;
    case SDLK_KP_9:
        return LumenEngine::EKeys::NumPad9;
    case SDLK_KP_0:
        return LumenEngine::EKeys::NumPad0;
    case SDLK_KP_PERIOD:
        return LumenEngine::EKeys::NumPadPeriod;
    case SDLK_APPLICATION:
        return LumenEngine::EKeys::Application;
    case SDLK_POWER:
        return LumenEngine::EKeys::Power;
    case SDLK_KP_EQUALS:
        return LumenEngine::EKeys::NumPadEquals;
    case SDLK_F13:
        return LumenEngine::EKeys::F13;
    case SDLK_F14:
        return LumenEngine::EKeys::F14;
    case SDLK_F15:
        return LumenEngine::EKeys::F15;
    case SDLK_F16:
        return LumenEngine::EKeys::F16;
    case SDLK_F17:
        return LumenEngine::EKeys::F17;
    case SDLK_F18:
        return LumenEngine::EKeys::F18;
    case SDLK_F19:
        return LumenEngine::EKeys::F19;
    case SDLK_F20:
        return LumenEngine::EKeys::F20;
    case SDLK_F21:
        return LumenEngine::EKeys::F21;
    case SDLK_F22:
        return LumenEngine::EKeys::F22;
    case SDLK_F23:
        return LumenEngine::EKeys::F23;
    case SDLK_F24:
        return LumenEngine::EKeys::F24;
    case SDLK_EXECUTE:
        return LumenEngine::EKeys::Execute;
    case SDLK_HELP:
        return LumenEngine::EKeys::Help;
    case SDLK_MENU:
        return LumenEngine::EKeys::Menu;
    case SDLK_SELECT:
        return LumenEngine::EKeys::Select;
    case SDLK_STOP:
        return LumenEngine::EKeys::Stop;
    case SDLK_AGAIN:
        return LumenEngine::EKeys::Again;
    case SDLK_UNDO:
        return LumenEngine::EKeys::Undo;
    case SDLK_CUT:
        return LumenEngine::EKeys::Cut;
    case SDLK_COPY:
        return LumenEngine::EKeys::Copy;
    case SDLK_PASTE:
        return LumenEngine::EKeys::Paste;
    case SDLK_FIND:
        return LumenEngine::EKeys::Find;
    case SDLK_MUTE:
        return LumenEngine::EKeys::Mute;
    case SDLK_VOLUMEUP:
        return LumenEngine::EKeys::VolumeUp;
    case SDLK_VOLUMEDOWN:
        return LumenEngine::EKeys::VolumeDown;
    case SDLK_KP_COMMA:
        return LumenEngine::EKeys::NumPadComma;
    case SDLK_KP_EQUALSAS400:
        return LumenEngine::EKeys::NumPadEqualsAs400;
    case SDLK_ALTERASE:
        return LumenEngine::EKeys::AltErase;
    case SDLK_SYSREQ:
        return LumenEngine::EKeys::SysReq;
    case SDLK_CANCEL:
        return LumenEngine::EKeys::Cancel;
    case SDLK_CLEAR:
        return LumenEngine::EKeys::Clear;
    case SDLK_PRIOR:
        return LumenEngine::EKeys::Prior;
    case SDLK_RETURN2:
        return LumenEngine::EKeys::Return2;
    case SDLK_SEPARATOR:
        return LumenEngine::EKeys::Separator;
    case SDLK_OUT:
        return LumenEngine::EKeys::Out;
    case SDLK_OPER:
        return LumenEngine::EKeys::Oper;
    case SDLK_CLEARAGAIN:
        return LumenEngine::EKeys::ClearAgain;
    case SDLK_CRSEL:
        return LumenEngine::EKeys::CrSel;
    case SDLK_EXSEL:
        return LumenEngine::EKeys::ExSel;
    case SDLK_KP_00:
        return LumenEngine::EKeys::NumPad00;
    case SDLK_KP_000:
        return LumenEngine::EKeys::NumPad000;
    case SDLK_THOUSANDSSEPARATOR:
        return LumenEngine::EKeys::ThousandsSeparator;
    case SDLK_DECIMALSEPARATOR:
        return LumenEngine::EKeys::DecimalSeparator;
    case SDLK_CURRENCYUNIT:
        return LumenEngine::EKeys::CurrencyUnit;
    case SDLK_CURRENCYSUBUNIT:
        return LumenEngine::EKeys::CurrencySubUnit;
    case SDLK_KP_LEFTPAREN:
        return LumenEngine::EKeys::NumPadLeftParenthesis;
    case SDLK_KP_RIGHTPAREN:
        return LumenEngine::EKeys::NumPadRightParenthesis;
    case SDLK_KP_LEFTBRACE:
        return LumenEngine::EKeys::NumPadLeftBrace;
    case SDLK_KP_RIGHTBRACE:
        return LumenEngine::EKeys::NumPadRightBrace;
    case SDLK_KP_TAB:
        return LumenEngine::EKeys::NumPadTab;
    case SDLK_KP_BACKSPACE:
        return LumenEngine::EKeys::NumPadBackspace;
    case SDLK_KP_A:
        return LumenEngine::EKeys::NumPadA;
    case SDLK_KP_B:
        return LumenEngine::EKeys::NumPadB;
    case SDLK_KP_C:
        return LumenEngine::EKeys::NumPadC;
    case SDLK_KP_D:
        return LumenEngine::EKeys::NumPadD;
    case SDLK_KP_E:
        return LumenEngine::EKeys::NumPadE;
    case SDLK_KP_F:
        return LumenEngine::EKeys::NumPadF;
    case SDLK_KP_XOR:
        return LumenEngine::EKeys::NumPadXor;
    case SDLK_KP_POWER:
        return LumenEngine::EKeys::NumPadPower;
    case SDLK_KP_PERCENT:
        return LumenEngine::EKeys::NumPadPercent;
    case SDLK_KP_LESS:
        return LumenEngine::EKeys::NumPadLess;
    case SDLK_KP_GREATER:
        return LumenEngine::EKeys::NumPadGreater;
    case SDLK_KP_AMPERSAND:
        return LumenEngine::EKeys::NumPadAmpersand;
    case SDLK_KP_DBLAMPERSAND:
        return LumenEngine::EKeys::NumPadDoubleAmpersand;
    case SDLK_KP_VERTICALBAR:
        return LumenEngine::EKeys::NumPadVerticalBar;
    case SDLK_KP_DBLVERTICALBAR:
        return LumenEngine::EKeys::NumPadDoubleVerticalBar;
    case SDLK_KP_COLON:
        return LumenEngine::EKeys::NumPadColon;
    case SDLK_KP_HASH:
        return LumenEngine::EKeys::NumPadHash;
    case SDLK_KP_SPACE:
        return LumenEngine::EKeys::NumPadSpace;
    case SDLK_KP_AT:
        return LumenEngine::EKeys::NumPadAt;
    case SDLK_KP_EXCLAM:
        return LumenEngine::EKeys::NumPadExclamation;
    case SDLK_KP_MEMSTORE:
        return LumenEngine::EKeys::NumPadMemStore;
    case SDLK_KP_MEMRECALL:
        return LumenEngine::EKeys::NumPadMemRecall;
    case SDLK_KP_MEMCLEAR:
        return LumenEngine::EKeys::NumPadMemClear;
    case SDLK_KP_MEMADD:
        return LumenEngine::EKeys::NumPadMemAdd;
    case SDLK_KP_MEMSUBTRACT:
        return LumenEngine::EKeys::NumPadMemSubtract;
    case SDLK_KP_MEMMULTIPLY:
        return LumenEngine::EKeys::NumPadMemMultiply;
    case SDLK_KP_MEMDIVIDE:
        return LumenEngine::EKeys::NumPadMemDivide;
    case SDLK_KP_PLUSMINUS:
        return LumenEngine::EKeys::NumPadPlusMinus;
    case SDLK_KP_CLEAR:
        return LumenEngine::EKeys::NumPadClear;
    case SDLK_KP_CLEARENTRY:
        return LumenEngine::EKeys::NumPadClearEntry;
    case SDLK_KP_BINARY:
        return LumenEngine::EKeys::NumPadBinary;
    case SDLK_KP_OCTAL:
        return LumenEngine::EKeys::NumPadOctal;
    case SDLK_KP_DECIMAL:
        return LumenEngine::EKeys::NumPadDecimal;
    case SDLK_KP_HEXADECIMAL:
        return LumenEngine::EKeys::NumPadHexadecimal;
    case SDLK_LCTRL:
        return LumenEngine::EKeys::LeftControl;
    case SDLK_LSHIFT:
        return LumenEngine::EKeys::LeftShift;
    case SDLK_LALT:
        return LumenEngine::EKeys::LeftAlt;
    case SDLK_LGUI:
        return LumenEngine::EKeys::LeftGui;
    case SDLK_RCTRL:
        return LumenEngine::EKeys::RightControl;
    case SDLK_RSHIFT:
        return LumenEngine::EKeys::RightShift;
    case SDLK_RALT:
        return LumenEngine::EKeys::RightAlt;
    case SDLK_RGUI:
        return LumenEngine::EKeys::RightGui;
    case SDLK_MODE:
        return LumenEngine::EKeys::Mode;
    case SDLK_SLEEP:
        return LumenEngine::EKeys::Sleep;
    case SDLK_WAKE:
        return LumenEngine::EKeys::Wake;
    case SDLK_CHANNEL_INCREMENT:
        return LumenEngine::EKeys::ChannelIncrement;
    case SDLK_CHANNEL_DECREMENT:
        return LumenEngine::EKeys::ChannelDecrement;
    case SDLK_MEDIA_PLAY:
        return LumenEngine::EKeys::MediaPlay;
    case SDLK_MEDIA_PAUSE:
        return LumenEngine::EKeys::MediaPause;
    case SDLK_MEDIA_RECORD:
        return LumenEngine::EKeys::MediaRecord;
    case SDLK_MEDIA_FAST_FORWARD:
        return LumenEngine::EKeys::MediaFastForward;
    case SDLK_MEDIA_REWIND:
        return LumenEngine::EKeys::MediaRewind;
    case SDLK_MEDIA_NEXT_TRACK:
        return LumenEngine::EKeys::MediaNextTrack;
    case SDLK_MEDIA_PREVIOUS_TRACK:
        return LumenEngine::EKeys::MediaPreviousTrack;
    case SDLK_MEDIA_STOP:
        return LumenEngine::EKeys::MediaStop;
    case SDLK_MEDIA_EJECT:
        return LumenEngine::EKeys::MediaEject;
    case SDLK_MEDIA_PLAY_PAUSE:
        return LumenEngine::EKeys::MediaPlayPause;
    case SDLK_MEDIA_SELECT:
        return LumenEngine::EKeys::MediaSelect;
    case SDLK_AC_NEW:
        return LumenEngine::EKeys::AcNew;
    case SDLK_AC_OPEN:
        return LumenEngine::EKeys::AcOpen;
    case SDLK_AC_CLOSE:
        return LumenEngine::EKeys::AcClose;
    case SDLK_AC_EXIT:
        return LumenEngine::EKeys::AcExit;
    case SDLK_AC_SAVE:
        return LumenEngine::EKeys::AcSave;
    case SDLK_AC_PRINT:
        return LumenEngine::EKeys::AcPrint;
    case SDLK_AC_PROPERTIES:
        return LumenEngine::EKeys::AcProperties;
    case SDLK_AC_SEARCH:
        return LumenEngine::EKeys::AcSearch;
    case SDLK_AC_HOME:
        return LumenEngine::EKeys::AcHome;
    case SDLK_AC_BACK:
        return LumenEngine::EKeys::AcBack;
    case SDLK_AC_FORWARD:
        return LumenEngine::EKeys::AcForward;
    case SDLK_AC_STOP:
        return LumenEngine::EKeys::AcStop;
    case SDLK_AC_REFRESH:
        return LumenEngine::EKeys::AcRefresh;
    case SDLK_AC_BOOKMARKS:
        return LumenEngine::EKeys::AcBookmarks;
    case SDLK_SOFTLEFT:
        return LumenEngine::EKeys::SoftLeft;
    case SDLK_SOFTRIGHT:
        return LumenEngine::EKeys::SoftRight;
    case SDLK_CALL:
        return LumenEngine::EKeys::Call;
    case SDLK_ENDCALL:
        return LumenEngine::EKeys::EndCall;
    case SDLK_LEFT_TAB:
        return LumenEngine::EKeys::LeftTab;
    case SDLK_LEVEL5_SHIFT:
        return LumenEngine::EKeys::Level5Shift;
    case SDLK_MULTI_KEY_COMPOSE:
        return LumenEngine::EKeys::MultiKeyCompose;
    case SDLK_LMETA:
        return LumenEngine::EKeys::LeftMeta;
    case SDLK_RMETA:
        return LumenEngine::EKeys::RightMeta;
    case SDLK_LHYPER:
        return LumenEngine::EKeys::LeftHyper;
    case SDLK_RHYPER:
        return LumenEngine::EKeys::RightHyper;
    default:
        return LumenEngine::EKeys::Unknown;
    }
}

static inline LumenEngine::EMouseButton::Type TranslateSDLMouseButtonToEMouseButton ( const LumenEngine::UInt8 Button )
{
    switch ( Button )
    {
    case SDL_BUTTON_LEFT:
        return LumenEngine::EMouseButton::Left;
    case SDL_BUTTON_MIDDLE:
        return LumenEngine::EMouseButton::Middle;
    case SDL_BUTTON_RIGHT:
        return LumenEngine::EMouseButton::Right;
    case SDL_BUTTON_X1:
        return LumenEngine::EMouseButton::Button4;
    case SDL_BUTTON_X2:
        return LumenEngine::EMouseButton::Button5;
    default:
        return LumenEngine::EMouseButton::Invalid;
    }
}

#endif
