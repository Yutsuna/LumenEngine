///
/// @file GenericApplicationInput.hpp
/// @brief Declaration of the FGenericApplicationInput struct for generic application input handling.
///

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

namespace Lumen
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

        /// Punctuation & Symbols
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

        /// Editing & Navigation
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

        /// Numpad
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

        /// Function Keys 13-24
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

        /// Modifiers
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

        /// Application / Multimedia / System
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

} // namespace Lumen

#if defined( LUMEN_PLATFORM_LINUX )

    #include <SDL3/SDL.h>

static inline Lumen::EKeys::Type TranslateSDLKeyCodeToEKeys ( const SDL_Keycode KeyCode )
{
    switch ( KeyCode )
    {
    case SDLK_UNKNOWN:
        return Lumen::EKeys::Unknown;
    case SDLK_RETURN:
        return Lumen::EKeys::Enter;
    case SDLK_ESCAPE:
        return Lumen::EKeys::Escape;
    case SDLK_BACKSPACE:
        return Lumen::EKeys::Backspace;
    case SDLK_TAB:
        return Lumen::EKeys::Tab;
    case SDLK_SPACE:
        return Lumen::EKeys::Spacebar;
    case SDLK_EXCLAIM:
        return Lumen::EKeys::Exclamation;
    case SDLK_DBLAPOSTROPHE:
        return Lumen::EKeys::DoubleApostrophe;
    case SDLK_HASH:
        return Lumen::EKeys::Hash;
    case SDLK_DOLLAR:
        return Lumen::EKeys::Dollar;
    case SDLK_PERCENT:
        return Lumen::EKeys::Percent;
    case SDLK_AMPERSAND:
        return Lumen::EKeys::Ampersand;
    case SDLK_APOSTROPHE:
        return Lumen::EKeys::Apostrophe;
    case SDLK_LEFTPAREN:
        return Lumen::EKeys::LeftParenthesis;
    case SDLK_RIGHTPAREN:
        return Lumen::EKeys::RightParenthesis;
    case SDLK_ASTERISK:
        return Lumen::EKeys::Asterisk;
    case SDLK_PLUS:
        return Lumen::EKeys::Plus;
    case SDLK_COMMA:
        return Lumen::EKeys::Comma;
    case SDLK_MINUS:
        return Lumen::EKeys::Minus;
    case SDLK_PERIOD:
        return Lumen::EKeys::Period;
    case SDLK_SLASH:
        return Lumen::EKeys::Slash;
    case SDLK_0:
        return Lumen::EKeys::Num0;
    case SDLK_1:
        return Lumen::EKeys::Num1;
    case SDLK_2:
        return Lumen::EKeys::Num2;
    case SDLK_3:
        return Lumen::EKeys::Num3;
    case SDLK_4:
        return Lumen::EKeys::Num4;
    case SDLK_5:
        return Lumen::EKeys::Num5;
    case SDLK_6:
        return Lumen::EKeys::Num6;
    case SDLK_7:
        return Lumen::EKeys::Num7;
    case SDLK_8:
        return Lumen::EKeys::Num8;
    case SDLK_9:
        return Lumen::EKeys::Num9;
    case SDLK_COLON:
        return Lumen::EKeys::Colon;
    case SDLK_SEMICOLON:
        return Lumen::EKeys::Semicolon;
    case SDLK_LESS:
        return Lumen::EKeys::Less;
    case SDLK_EQUALS:
        return Lumen::EKeys::Equals;
    case SDLK_GREATER:
        return Lumen::EKeys::Greater;
    case SDLK_QUESTION:
        return Lumen::EKeys::Question;
    case SDLK_AT:
        return Lumen::EKeys::At;
    case SDLK_LEFTBRACKET:
        return Lumen::EKeys::LeftBracket;
    case SDLK_BACKSLASH:
        return Lumen::EKeys::Backslash;
    case SDLK_RIGHTBRACKET:
        return Lumen::EKeys::RightBracket;
    case SDLK_CARET:
        return Lumen::EKeys::Caret;
    case SDLK_UNDERSCORE:
        return Lumen::EKeys::Underscore;
    case SDLK_GRAVE:
        return Lumen::EKeys::GraveAccent;
    case SDLK_A:
        return Lumen::EKeys::A;
    case SDLK_B:
        return Lumen::EKeys::B;
    case SDLK_C:
        return Lumen::EKeys::C;
    case SDLK_D:
        return Lumen::EKeys::D;
    case SDLK_E:
        return Lumen::EKeys::E;
    case SDLK_F:
        return Lumen::EKeys::F;
    case SDLK_G:
        return Lumen::EKeys::G;
    case SDLK_H:
        return Lumen::EKeys::H;
    case SDLK_I:
        return Lumen::EKeys::I;
    case SDLK_J:
        return Lumen::EKeys::J;
    case SDLK_K:
        return Lumen::EKeys::K;
    case SDLK_L:
        return Lumen::EKeys::L;
    case SDLK_M:
        return Lumen::EKeys::M;
    case SDLK_N:
        return Lumen::EKeys::N;
    case SDLK_O:
        return Lumen::EKeys::O;
    case SDLK_P:
        return Lumen::EKeys::P;
    case SDLK_Q:
        return Lumen::EKeys::Q;
    case SDLK_R:
        return Lumen::EKeys::R;
    case SDLK_S:
        return Lumen::EKeys::S;
    case SDLK_T:
        return Lumen::EKeys::T;
    case SDLK_U:
        return Lumen::EKeys::U;
    case SDLK_V:
        return Lumen::EKeys::V;
    case SDLK_W:
        return Lumen::EKeys::W;
    case SDLK_X:
        return Lumen::EKeys::X;
    case SDLK_Y:
        return Lumen::EKeys::Y;
    case SDLK_Z:
        return Lumen::EKeys::Z;
    case SDLK_LEFTBRACE:
        return Lumen::EKeys::LeftBrace;
    case SDLK_PIPE:
        return Lumen::EKeys::Pipe;
    case SDLK_RIGHTBRACE:
        return Lumen::EKeys::RightBrace;
    case SDLK_TILDE:
        return Lumen::EKeys::Tilde;
    case SDLK_DELETE:
        return Lumen::EKeys::Delete;
    case SDLK_PLUSMINUS:
        return Lumen::EKeys::PlusMinus;
    case SDLK_CAPSLOCK:
        return Lumen::EKeys::CapsLock;
    case SDLK_F1:
        return Lumen::EKeys::F1;
    case SDLK_F2:
        return Lumen::EKeys::F2;
    case SDLK_F3:
        return Lumen::EKeys::F3;
    case SDLK_F4:
        return Lumen::EKeys::F4;
    case SDLK_F5:
        return Lumen::EKeys::F5;
    case SDLK_F6:
        return Lumen::EKeys::F6;
    case SDLK_F7:
        return Lumen::EKeys::F7;
    case SDLK_F8:
        return Lumen::EKeys::F8;
    case SDLK_F9:
        return Lumen::EKeys::F9;
    case SDLK_F10:
        return Lumen::EKeys::F10;
    case SDLK_F11:
        return Lumen::EKeys::F11;
    case SDLK_F12:
        return Lumen::EKeys::F12;
    case SDLK_PRINTSCREEN:
        return Lumen::EKeys::PrintScreen;
    case SDLK_SCROLLLOCK:
        return Lumen::EKeys::ScrollLock;
    case SDLK_PAUSE:
        return Lumen::EKeys::Pause;
    case SDLK_INSERT:
        return Lumen::EKeys::Insert;
    case SDLK_HOME:
        return Lumen::EKeys::Home;
    case SDLK_PAGEUP:
        return Lumen::EKeys::PageUp;
    case SDLK_END:
        return Lumen::EKeys::End;
    case SDLK_PAGEDOWN:
        return Lumen::EKeys::PageDown;
    case SDLK_RIGHT:
        return Lumen::EKeys::Right;
    case SDLK_LEFT:
        return Lumen::EKeys::Left;
    case SDLK_DOWN:
        return Lumen::EKeys::Down;
    case SDLK_UP:
        return Lumen::EKeys::Up;
    case SDLK_NUMLOCKCLEAR:
        return Lumen::EKeys::NumLock;
    case SDLK_KP_DIVIDE:
        return Lumen::EKeys::NumPadDivide;
    case SDLK_KP_MULTIPLY:
        return Lumen::EKeys::NumPadMultiply;
    case SDLK_KP_MINUS:
        return Lumen::EKeys::NumPadMinus;
    case SDLK_KP_PLUS:
        return Lumen::EKeys::NumPadPlus;
    case SDLK_KP_ENTER:
        return Lumen::EKeys::NumPadEnter;
    case SDLK_KP_1:
        return Lumen::EKeys::NumPad1;
    case SDLK_KP_2:
        return Lumen::EKeys::NumPad2;
    case SDLK_KP_3:
        return Lumen::EKeys::NumPad3;
    case SDLK_KP_4:
        return Lumen::EKeys::NumPad4;
    case SDLK_KP_5:
        return Lumen::EKeys::NumPad5;
    case SDLK_KP_6:
        return Lumen::EKeys::NumPad6;
    case SDLK_KP_7:
        return Lumen::EKeys::NumPad7;
    case SDLK_KP_8:
        return Lumen::EKeys::NumPad8;
    case SDLK_KP_9:
        return Lumen::EKeys::NumPad9;
    case SDLK_KP_0:
        return Lumen::EKeys::NumPad0;
    case SDLK_KP_PERIOD:
        return Lumen::EKeys::NumPadPeriod;
    case SDLK_APPLICATION:
        return Lumen::EKeys::Application;
    case SDLK_POWER:
        return Lumen::EKeys::Power;
    case SDLK_KP_EQUALS:
        return Lumen::EKeys::NumPadEquals;
    case SDLK_F13:
        return Lumen::EKeys::F13;
    case SDLK_F14:
        return Lumen::EKeys::F14;
    case SDLK_F15:
        return Lumen::EKeys::F15;
    case SDLK_F16:
        return Lumen::EKeys::F16;
    case SDLK_F17:
        return Lumen::EKeys::F17;
    case SDLK_F18:
        return Lumen::EKeys::F18;
    case SDLK_F19:
        return Lumen::EKeys::F19;
    case SDLK_F20:
        return Lumen::EKeys::F20;
    case SDLK_F21:
        return Lumen::EKeys::F21;
    case SDLK_F22:
        return Lumen::EKeys::F22;
    case SDLK_F23:
        return Lumen::EKeys::F23;
    case SDLK_F24:
        return Lumen::EKeys::F24;
    case SDLK_EXECUTE:
        return Lumen::EKeys::Execute;
    case SDLK_HELP:
        return Lumen::EKeys::Help;
    case SDLK_MENU:
        return Lumen::EKeys::Menu;
    case SDLK_SELECT:
        return Lumen::EKeys::Select;
    case SDLK_STOP:
        return Lumen::EKeys::Stop;
    case SDLK_AGAIN:
        return Lumen::EKeys::Again;
    case SDLK_UNDO:
        return Lumen::EKeys::Undo;
    case SDLK_CUT:
        return Lumen::EKeys::Cut;
    case SDLK_COPY:
        return Lumen::EKeys::Copy;
    case SDLK_PASTE:
        return Lumen::EKeys::Paste;
    case SDLK_FIND:
        return Lumen::EKeys::Find;
    case SDLK_MUTE:
        return Lumen::EKeys::Mute;
    case SDLK_VOLUMEUP:
        return Lumen::EKeys::VolumeUp;
    case SDLK_VOLUMEDOWN:
        return Lumen::EKeys::VolumeDown;
    case SDLK_KP_COMMA:
        return Lumen::EKeys::NumPadComma;
    case SDLK_KP_EQUALSAS400:
        return Lumen::EKeys::NumPadEqualsAs400;
    case SDLK_ALTERASE:
        return Lumen::EKeys::AltErase;
    case SDLK_SYSREQ:
        return Lumen::EKeys::SysReq;
    case SDLK_CANCEL:
        return Lumen::EKeys::Cancel;
    case SDLK_CLEAR:
        return Lumen::EKeys::Clear;
    case SDLK_PRIOR:
        return Lumen::EKeys::Prior;
    case SDLK_RETURN2:
        return Lumen::EKeys::Return2;
    case SDLK_SEPARATOR:
        return Lumen::EKeys::Separator;
    case SDLK_OUT:
        return Lumen::EKeys::Out;
    case SDLK_OPER:
        return Lumen::EKeys::Oper;
    case SDLK_CLEARAGAIN:
        return Lumen::EKeys::ClearAgain;
    case SDLK_CRSEL:
        return Lumen::EKeys::CrSel;
    case SDLK_EXSEL:
        return Lumen::EKeys::ExSel;
    case SDLK_KP_00:
        return Lumen::EKeys::NumPad00;
    case SDLK_KP_000:
        return Lumen::EKeys::NumPad000;
    case SDLK_THOUSANDSSEPARATOR:
        return Lumen::EKeys::ThousandsSeparator;
    case SDLK_DECIMALSEPARATOR:
        return Lumen::EKeys::DecimalSeparator;
    case SDLK_CURRENCYUNIT:
        return Lumen::EKeys::CurrencyUnit;
    case SDLK_CURRENCYSUBUNIT:
        return Lumen::EKeys::CurrencySubUnit;
    case SDLK_KP_LEFTPAREN:
        return Lumen::EKeys::NumPadLeftParenthesis;
    case SDLK_KP_RIGHTPAREN:
        return Lumen::EKeys::NumPadRightParenthesis;
    case SDLK_KP_LEFTBRACE:
        return Lumen::EKeys::NumPadLeftBrace;
    case SDLK_KP_RIGHTBRACE:
        return Lumen::EKeys::NumPadRightBrace;
    case SDLK_KP_TAB:
        return Lumen::EKeys::NumPadTab;
    case SDLK_KP_BACKSPACE:
        return Lumen::EKeys::NumPadBackspace;
    case SDLK_KP_A:
        return Lumen::EKeys::NumPadA;
    case SDLK_KP_B:
        return Lumen::EKeys::NumPadB;
    case SDLK_KP_C:
        return Lumen::EKeys::NumPadC;
    case SDLK_KP_D:
        return Lumen::EKeys::NumPadD;
    case SDLK_KP_E:
        return Lumen::EKeys::NumPadE;
    case SDLK_KP_F:
        return Lumen::EKeys::NumPadF;
    case SDLK_KP_XOR:
        return Lumen::EKeys::NumPadXor;
    case SDLK_KP_POWER:
        return Lumen::EKeys::NumPadPower;
    case SDLK_KP_PERCENT:
        return Lumen::EKeys::NumPadPercent;
    case SDLK_KP_LESS:
        return Lumen::EKeys::NumPadLess;
    case SDLK_KP_GREATER:
        return Lumen::EKeys::NumPadGreater;
    case SDLK_KP_AMPERSAND:
        return Lumen::EKeys::NumPadAmpersand;
    case SDLK_KP_DBLAMPERSAND:
        return Lumen::EKeys::NumPadDoubleAmpersand;
    case SDLK_KP_VERTICALBAR:
        return Lumen::EKeys::NumPadVerticalBar;
    case SDLK_KP_DBLVERTICALBAR:
        return Lumen::EKeys::NumPadDoubleVerticalBar;
    case SDLK_KP_COLON:
        return Lumen::EKeys::NumPadColon;
    case SDLK_KP_HASH:
        return Lumen::EKeys::NumPadHash;
    case SDLK_KP_SPACE:
        return Lumen::EKeys::NumPadSpace;
    case SDLK_KP_AT:
        return Lumen::EKeys::NumPadAt;
    case SDLK_KP_EXCLAM:
        return Lumen::EKeys::NumPadExclamation;
    case SDLK_KP_MEMSTORE:
        return Lumen::EKeys::NumPadMemStore;
    case SDLK_KP_MEMRECALL:
        return Lumen::EKeys::NumPadMemRecall;
    case SDLK_KP_MEMCLEAR:
        return Lumen::EKeys::NumPadMemClear;
    case SDLK_KP_MEMADD:
        return Lumen::EKeys::NumPadMemAdd;
    case SDLK_KP_MEMSUBTRACT:
        return Lumen::EKeys::NumPadMemSubtract;
    case SDLK_KP_MEMMULTIPLY:
        return Lumen::EKeys::NumPadMemMultiply;
    case SDLK_KP_MEMDIVIDE:
        return Lumen::EKeys::NumPadMemDivide;
    case SDLK_KP_PLUSMINUS:
        return Lumen::EKeys::NumPadPlusMinus;
    case SDLK_KP_CLEAR:
        return Lumen::EKeys::NumPadClear;
    case SDLK_KP_CLEARENTRY:
        return Lumen::EKeys::NumPadClearEntry;
    case SDLK_KP_BINARY:
        return Lumen::EKeys::NumPadBinary;
    case SDLK_KP_OCTAL:
        return Lumen::EKeys::NumPadOctal;
    case SDLK_KP_DECIMAL:
        return Lumen::EKeys::NumPadDecimal;
    case SDLK_KP_HEXADECIMAL:
        return Lumen::EKeys::NumPadHexadecimal;
    case SDLK_LCTRL:
        return Lumen::EKeys::LeftControl;
    case SDLK_LSHIFT:
        return Lumen::EKeys::LeftShift;
    case SDLK_LALT:
        return Lumen::EKeys::LeftAlt;
    case SDLK_LGUI:
        return Lumen::EKeys::LeftGui;
    case SDLK_RCTRL:
        return Lumen::EKeys::RightControl;
    case SDLK_RSHIFT:
        return Lumen::EKeys::RightShift;
    case SDLK_RALT:
        return Lumen::EKeys::RightAlt;
    case SDLK_RGUI:
        return Lumen::EKeys::RightGui;
    case SDLK_MODE:
        return Lumen::EKeys::Mode;
    case SDLK_SLEEP:
        return Lumen::EKeys::Sleep;
    case SDLK_WAKE:
        return Lumen::EKeys::Wake;
    case SDLK_CHANNEL_INCREMENT:
        return Lumen::EKeys::ChannelIncrement;
    case SDLK_CHANNEL_DECREMENT:
        return Lumen::EKeys::ChannelDecrement;
    case SDLK_MEDIA_PLAY:
        return Lumen::EKeys::MediaPlay;
    case SDLK_MEDIA_PAUSE:
        return Lumen::EKeys::MediaPause;
    case SDLK_MEDIA_RECORD:
        return Lumen::EKeys::MediaRecord;
    case SDLK_MEDIA_FAST_FORWARD:
        return Lumen::EKeys::MediaFastForward;
    case SDLK_MEDIA_REWIND:
        return Lumen::EKeys::MediaRewind;
    case SDLK_MEDIA_NEXT_TRACK:
        return Lumen::EKeys::MediaNextTrack;
    case SDLK_MEDIA_PREVIOUS_TRACK:
        return Lumen::EKeys::MediaPreviousTrack;
    case SDLK_MEDIA_STOP:
        return Lumen::EKeys::MediaStop;
    case SDLK_MEDIA_EJECT:
        return Lumen::EKeys::MediaEject;
    case SDLK_MEDIA_PLAY_PAUSE:
        return Lumen::EKeys::MediaPlayPause;
    case SDLK_MEDIA_SELECT:
        return Lumen::EKeys::MediaSelect;
    case SDLK_AC_NEW:
        return Lumen::EKeys::AcNew;
    case SDLK_AC_OPEN:
        return Lumen::EKeys::AcOpen;
    case SDLK_AC_CLOSE:
        return Lumen::EKeys::AcClose;
    case SDLK_AC_EXIT:
        return Lumen::EKeys::AcExit;
    case SDLK_AC_SAVE:
        return Lumen::EKeys::AcSave;
    case SDLK_AC_PRINT:
        return Lumen::EKeys::AcPrint;
    case SDLK_AC_PROPERTIES:
        return Lumen::EKeys::AcProperties;
    case SDLK_AC_SEARCH:
        return Lumen::EKeys::AcSearch;
    case SDLK_AC_HOME:
        return Lumen::EKeys::AcHome;
    case SDLK_AC_BACK:
        return Lumen::EKeys::AcBack;
    case SDLK_AC_FORWARD:
        return Lumen::EKeys::AcForward;
    case SDLK_AC_STOP:
        return Lumen::EKeys::AcStop;
    case SDLK_AC_REFRESH:
        return Lumen::EKeys::AcRefresh;
    case SDLK_AC_BOOKMARKS:
        return Lumen::EKeys::AcBookmarks;
    case SDLK_SOFTLEFT:
        return Lumen::EKeys::SoftLeft;
    case SDLK_SOFTRIGHT:
        return Lumen::EKeys::SoftRight;
    case SDLK_CALL:
        return Lumen::EKeys::Call;
    case SDLK_ENDCALL:
        return Lumen::EKeys::EndCall;
    case SDLK_LEFT_TAB:
        return Lumen::EKeys::LeftTab;
    case SDLK_LEVEL5_SHIFT:
        return Lumen::EKeys::Level5Shift;
    case SDLK_MULTI_KEY_COMPOSE:
        return Lumen::EKeys::MultiKeyCompose;
    case SDLK_LMETA:
        return Lumen::EKeys::LeftMeta;
    case SDLK_RMETA:
        return Lumen::EKeys::RightMeta;
    case SDLK_LHYPER:
        return Lumen::EKeys::LeftHyper;
    case SDLK_RHYPER:
        return Lumen::EKeys::RightHyper;
    default:
        return Lumen::EKeys::Unknown;
    }
}

static inline Lumen::EMouseButton::Type TranslateSDLMouseButtonToEMouseButton ( const Lumen::UInt8 Button )
{
    switch ( Button )
    {
    case SDL_BUTTON_LEFT:
        return Lumen::EMouseButton::Left;
    case SDL_BUTTON_MIDDLE:
        return Lumen::EMouseButton::Middle;
    case SDL_BUTTON_RIGHT:
        return Lumen::EMouseButton::Right;
    case SDL_BUTTON_X1:
        return Lumen::EMouseButton::Button4;
    case SDL_BUTTON_X2:
        return Lumen::EMouseButton::Button5;
    default:
        return Lumen::EMouseButton::Invalid;
    }
}

#endif
