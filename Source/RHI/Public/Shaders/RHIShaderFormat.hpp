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
 */
enum class ERHIShaderFormat : UInt8
{
    Null,
    SPIRV,
    DXIL,
    DXBC,
    GLSL,
    HLSL
};
/** @return True for a defined format value. */
[[nodiscard]] RHI_EXPORT Bool IsKnownRHIShaderFormat ( ERHIShaderFormat InFormat ) noexcept;
} // namespace Lumen
