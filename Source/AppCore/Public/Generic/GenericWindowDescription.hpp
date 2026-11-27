/**
 * @file GenericWindowDescription.hpp
 * @brief Declaration of the FGenericWindowDescription struct for window configuration.
 */

#pragma once

#include <AppCoreExport.hpp>

#include <Core/CoreTypes.hpp>

#include <Meta/Reflection.hpp>

#include <String/String.hpp>

#include <Maths/Vec.hpp>

namespace Lumen
{

/**
 * @enum EWindowGraphicsApi
 * @brief Selects the graphics API for which the native window must be created.
 */
enum class EWindowGraphicsApi : UInt8
{
    None,
    OpenGL,
    Vulkan
};

namespace EWindowMode
{

    /**
     * @brief Enumeration of possible window modes.
     */
    enum Type : Int32 /// NOLINT(performance-enum-size)
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
     * @return The reflected enumerator name ("<unknown>" for out-of-range values), or the explicit
     *         mapping when reflection is unavailable.
     */
    [[nodiscard]] static constexpr FStringView ToString ( const Type InWindowMode ) noexcept
    {
#if LUMEN_HAS_REFLECTION
        return Meta::EnumName( InWindowMode );
#else
        /** INFO: Reflection is unavailable; fall back to the explicit mapping. */
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
#endif
    }

} // namespace EWindowMode

namespace /// NOLINT(cert-dcl59-cpp)
{

    constexpr const AnsiChar *const DefaultWindowTitle = "Lumen Engine";

    constexpr Int32 DefaultWindowWidth  = 1280;
    constexpr Int32 DefaultWindowHeight = 720;

    constexpr Int32 DefaultWindowPosX = 100;
    constexpr Int32 DefaultWindowPosY = 100;

} // namespace

/**
 * @struct FGenericWindowDescription
 * @brief Structure describing the properties of a generic window.
 */
struct APPCORE_EXPORT FGenericWindowDescription
{
    /** Title of the window */
    FString Title = FString{ DefaultWindowTitle };
    /** Initial position of the window on screen */
    Maths::FVec2i Position = Maths::FVec2i( DefaultWindowPosX, DefaultWindowPosY );
    /** Initial size of the window */
    Maths::FVec2i Size = Maths::FVec2i( DefaultWindowWidth, DefaultWindowHeight );
    /** Initial window mode */
    EWindowMode::Type WindowMode = EWindowMode::Windowed;
    /** Graphics API required from the native window */
    EWindowGraphicsApi GraphicsApi = EWindowGraphicsApi::None;
    /** Is the window resizable by the user */
    Bool bIsResizable = true;
    /** Is the window initially visible on screen */
    Bool bIsVisible = true;
};

} // namespace Lumen
