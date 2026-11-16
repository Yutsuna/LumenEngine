/**
 * @file RHIShaderStage.hpp
 * @brief Defines shader stages and stage masks.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/EnumFlags.hpp>

namespace Lumen
{

/**
 * @enum ERHIShaderStage
 * @brief Shader stage bits used by shaders and bindings.
 */
enum class ERHIShaderStage : UInt32
{
    None          = 0U,
    Vertex        = 1U << 0U,
    Pixel         = 1U << 1U,
    Compute       = 1U << 2U,
    Geometry      = 1U << 3U,
    Hull          = 1U << 4U,
    Domain        = 1U << 5U,
    Amplification = 1U << 6U,
    Mesh          = 1U << 7U,
    AllGraphics   = ( 1U << 0U ) | ( 1U << 1U ) | ( 1U << 3U ) | ( 1U << 4U ) | ( 1U << 5U ) | ( 1U << 6U ) | ( 1U << 7U ),
    All = ( 1U << 0U ) | ( 1U << 1U ) | ( 1U << 2U ) | ( 1U << 3U ) | ( 1U << 4U ) | ( 1U << 5U ) | ( 1U << 6U ) | ( 1U << 7U ),
};

LUMEN_ENUM_FLAGS( ERHIShaderStage );

} // namespace Lumen
