/**
 * @file RHIShaderFormat.hpp
 * @brief Defines portable shader payload formats.
 */

#pragma once
#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>
namespace Lumen
{
/**
 * @enum ERHIShaderFormat
 * @brief Portable shader payloads understood by backends.
 * @details Each value identifies a payload representation a backend may accept.
 *          Backends advertise the formats they support through capabilities,
 *          and callers must negotiate a format before creating a shader package.
 */
enum class ERHIShaderFormat : UInt8
{
    /** @brief No payload; the format is unspecified. */
    Null,
    /** @brief Portable intermediate bytecode payload. */
    SPIRV,
    /** @brief Backend-compiled bytecode payload. */
    DXIL,
    /** @brief Legacy backend-compiled bytecode payload. */
    DXBC,
    /** @brief High-level shading-language source payload. */
    GLSL,
    /** @brief High-level shading-language source payload with a C-like syntax. */
    HLSL
};

/**
 * @brief Classifies a shader format value.
 * @param InFormat Format value to examine.
 * @return True when InFormat names a defined enumerator; false for an integer
 *         value outside the defined range.
 */
[[nodiscard]] RHI_EXPORT Bool IsKnownRHIShaderFormat ( ERHIShaderFormat InFormat ) noexcept;
} // namespace Lumen
