///
/// @file GenericWindowDescription.hpp
/// @brief Declaration of the FGenericWindowDescription struct for window configuration.
///

#pragma once

#include <Core/CoreTypes.hpp>

#include <String/String.hpp>

#include <Maths/Vec.hpp>

namespace Lumen
{

namespace EWindowMode
{

    ///
    /// @brief Enumeration of possible window modes.
    ///
    enum Type : Int32
    {
        /// True fullscreen window mode
        Fullscreen,
        /// Borderless windowed fullscreen mode
        WindowedFullscreen,
        /// Standard windowed mode
        Windowed,

        /// Number of window modes
        NumWindowModes
    };

    ///
    /// @brief Convert a window mode enum value to its string representation.
    ///
    static constexpr const AnsiChar *ToString ( const Type InWindowMode )
    {
        switch ( InWindowMode )
        {
        case Fullscreen:
            return "Fullscreen";
        case WindowedFullscreen:
            return "WindowedFullscreen";
        case Windowed:
            return "Windowed";
        default:
            return "Unknown";
        }
    }

} // namespace EWindowMode

namespace
{

    constexpr const AnsiChar *const DefaultWindowTitle = "Lumen Engine";

    constexpr Int32 DefaultWindowWidth  = 1280;
    constexpr Int32 DefaultWindowHeight = 720;

    constexpr Int32 DefaultWindowPosX = 100;
    constexpr Int32 DefaultWindowPosY = 100;

} // namespace

///
/// @brief Structure describing the properties of a generic window.
///
struct FGenericWindowDescription
{
    /// Title of the window
    FString Title = FString{ DefaultWindowTitle };
    /// Initial position of the window on screen
    Maths::FVec2i Position = Maths::FVec2i( DefaultWindowPosX, DefaultWindowPosY );
    /// Initial size of the window
    Maths::FVec2i Size = Maths::FVec2i( DefaultWindowWidth, DefaultWindowHeight );
    /// Initial window mode
    EWindowMode::Type WindowMode = EWindowMode::Windowed;
    /// Is the window resizable by the user
    Bool bIsResizable = true;
    /// Is the window initially visible on screen
    Bool bIsVisible = true;
};

} // namespace Lumen
