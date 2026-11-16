/**
 * @file RHIFlags.hpp
 * @brief Defines common object creation flags.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/EnumFlags.hpp>

namespace Lumen
{

/** @enum ERHIObjectFlags @brief Portable flags shared by creatable objects. */
enum class ERHIObjectFlags : UInt32
{
    None                 = 0U,
    Shared               = 1U << 0U,
    Transient            = 1U << 1U,
    DisableDebugTracking = 1U << 2U,
};

LUMEN_ENUM_FLAGS( ERHIObjectFlags );

} // namespace Lumen
