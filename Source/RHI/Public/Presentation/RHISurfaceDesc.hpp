/**
 * @file RHISurfaceDesc.hpp
 * @brief Defines the isolated, backend-neutral surface payload used for presentation.
 */
#pragma once

#include <RHIExport.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIExtent.hpp>

namespace Lumen
{

/**
 * @enum ERHISurfaceType
 * @brief Portable category of the opaque payload carried by a surface descriptor.
 * @details The concrete backend binding is negotiated through capabilities; the
 *          RHI records only the category and the opaque handles it carries.
 */
enum class ERHISurfaceType : UInt8
{
    None,   ///< No payload.
    Null,   ///< Synthetic payload for headless and test backends.
    Window, ///< Opaque window handle, optionally paired with a display handle.
    Count
};

/**
 * @struct FRHISurfaceDesc
 * @brief Non-owning opaque surface handles and its initial pixel extent.
 * @details Handles are opaque integer values and are never dereferenced by the portable RHI.
 *          bRequiresDisplay declares whether the payload also carries a display handle.
 */
struct RHI_EXPORT FRHISurfaceDesc
{
    ERHISurfaceType Type  = ERHISurfaceType::None;
    UPtr NativeWindow     = 0U;
    UPtr NativeDisplay    = 0U;
    Bool bRequiresDisplay = false;
    FRHIExtent3D InitialExtent;

    /** @return Success when the payload category and required handles are coherent. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};

} // namespace Lumen
