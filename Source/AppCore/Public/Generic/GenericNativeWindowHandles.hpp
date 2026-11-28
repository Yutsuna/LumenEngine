/**
 * @file GenericNativeWindowHandles.hpp
 * @brief Declares portable, non-owning native window handle values.
 */
#pragma once

#include <AppCoreExport.hpp>
#include <Core/CoreTypes.hpp>

namespace Lumen
{

/**
 * @enum ENativeWindowSystem
 * @brief Identifies the window system that owns a set of opaque native handles.
 */
enum class ENativeWindowSystem : UInt8
{
    Unknown,
    X11,
    Wayland,
    Win32
};

/**
 * @struct FNativeWindowHandles
 * @brief Non-owning native window handles and drawable size, independent of SDL and rendering APIs.
 * @details NativeWindow is an X11 window ID, a Wayland surface or a Win32 HWND. NativeWindowAttachment
 *          carries the Wayland EGL window when SDL provides one; it is zero on X11 and Win32. None of
 *          the values may be dereferenced or cast without selecting the corresponding ENativeWindowSystem contract.
 */
struct APPCORE_EXPORT FNativeWindowHandles
{
    ENativeWindowSystem WindowSystem = ENativeWindowSystem::Unknown;
    UPtr NativeWindow                = 0U;
    UPtr NativeDisplay               = 0U;
    UPtr NativeWindowAttachment      = 0U;
    UInt32 Width                     = 0U;
    UInt32 Height                    = 0U;
};

} // namespace Lumen
