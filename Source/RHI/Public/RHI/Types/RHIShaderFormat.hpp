/**
 * @file RHIShaderFormat.hpp
 * @brief Defines portable shader payload formats.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
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
        /** @brief Null backend payload. */
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

    /** @return True when InFormat names a defined enumerator. */
    [[nodiscard]] RHI_EXPORT Bool IsKnownRHIShaderFormat ( ERHIShaderFormat InFormat ) noexcept;
} // namespace RHI

} // namespace Lumen
