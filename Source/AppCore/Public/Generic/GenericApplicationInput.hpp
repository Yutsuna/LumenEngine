/**
 * @file GenericApplicationInput.hpp
 * @brief Declaration of the FGenericApplicationInput struct for generic application input handling.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace EMouseButton
{

    /**
     * @enum EMouseButton::Type
     * @brief Enumeration of mouse button types.
     */
    enum Type : UInt8 /** NOLINT(performance-enum-size) */
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

    /**
     * @enum EModifierKey::Type
     * @brief Enumeration of modifier key types.
     */
    enum Type : UInt32 /** NOLINT(performance-enum-size) */
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

    /**
     * @enum EKeys::Type
     * @brief Enumeration of keyboard key types.
     */
    enum Type : UInt32 /** NOLINT(performance-enum-size) */
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

} // namespace Lumen
