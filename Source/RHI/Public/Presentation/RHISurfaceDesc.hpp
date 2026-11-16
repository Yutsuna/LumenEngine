/**
 * @file RHISurfaceDesc.hpp
 * @brief Defines the isolated native-window payload used for presentation.
 */
#pragma once

#include <RHIExport.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIExtent.hpp>

namespace Lumen
{

/**
 * @enum ERHISurfaceType
 * @brief Identifies the native handles carried by a surface descriptor.
 */
enum class ERHISurfaceType : UInt8
{
    None,
    Null,
    Win32,
    Xlib,
    Xcb,
    Wayland,
    Metal,
    Android,
    Count
};

/**
 * @struct FRHISurfaceDesc
 * @brief Non-owning native surface identifiers and its initial pixel extent.
 * @details Handles are opaque integer values and are never dereferenced by the portable RHI.
 */
struct RHI_EXPORT FRHISurfaceDesc
{
    ERHISurfaceType Type = ERHISurfaceType::None;
    UPtr NativeWindow    = 0U;
    UPtr NativeDisplay   = 0U;
    FRHIExtent3D InitialExtent;

    /** @return Success when the payload type and required handles are coherent. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};

} // namespace Lumen
