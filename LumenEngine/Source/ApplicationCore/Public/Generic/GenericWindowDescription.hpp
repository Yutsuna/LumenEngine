/**
 * @file GenericWindowDescription.hpp
 * @brief Declaration of the FGenericWindowDescription struct for window configuration.
 */

#pragma once

#include "Container/String.hpp"
#include "CoreTypes.hpp"

#include "Maths/Vector.hpp"

namespace LumenEngine
{

namespace EWindowMode
{

    /**
     * @brief Enumeration of possible window modes.
     */
    enum Type : Int32
    {
        /** True fullscreen window mode */
        Fullscreen,
        /** Borderless windowed fullscreen mode */
        WindowedFullscreen,
        /** Standard windowed mode */
        Windowed,

        /** Number of window modes */
        NumWindowModes
    };

    /**
     * @brief Convert a window mode enum value to its string representation.
     */
    static inline constexpr const AnsiChar *const ToString ( const Type InWindowMode )
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

    static constexpr const AnsiChar *const DefaultWindowTitle = "Lumen Engine";

    static constexpr const Int32 DefaultWindowWidth  = 1280;
    static constexpr const Int32 DefaultWindowHeight = 720;

    static constexpr const Int32 DefaultWindowPosX = 100;
    static constexpr const Int32 DefaultWindowPosY = 100;

} // namespace

/**
 * @brief Structure describing the properties of a generic window.
 */
struct FGenericWindowDescription
{
    /** Title of the window */
    FString Title = FString{ DefaultWindowTitle };
    /** Initial position of the window on screen */
    Maths::FVec2i Position = Maths::FVec2i( DefaultWindowPosX, DefaultWindowPosY );
    /** Initial size of the window */
    Maths::FVec2i Size = Maths::FVec2i( DefaultWindowWidth, DefaultWindowHeight );
    /** Initial window mode */
    EWindowMode::Type WindowMode = EWindowMode::Windowed;
    /** Is the window resizable by the user */
    Bool bIsResizable = true;
    /** Is the window initially visible on screen */
    Bool bIsVisible = true;
};

} // namespace LumenEngine
