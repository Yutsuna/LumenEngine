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
 * @brief Portable shader payload representations understood by backends.
 * @details Each value identifies a representation a backend may accept, not a
 *          particular shading language or bytecode encoding. Backends advertise
 *          the representations they support through capabilities, and callers
 *          must negotiate a format before creating a shader package.
 */
enum class ERHIShaderFormat : UInt8
{
    /** @brief No payload; the format is unspecified. */
    Null,
    /** @brief Portable intermediate bytecode payload. */
    PortableBytecode,
    /** @brief Backend-compiled bytecode payload; the encoding is backend-defined. */
    BackendBytecode,
    /** @brief High-level shading-language source payload; the language is backend-defined. */
    Source,
    /** @brief Sentinel for range checks; not a usable payload. */
    Count
};

/**
 * @brief Classifies a shader format value.
 * @param InFormat Format value to examine.
 * @return True when InFormat names a defined enumerator; false for an integer
 *         value outside the defined range.
 */
[[nodiscard]] RHI_EXPORT Bool IsKnownRHIShaderFormat ( ERHIShaderFormat InFormat ) noexcept;
} // namespace Lumen
