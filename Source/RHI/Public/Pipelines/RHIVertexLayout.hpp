/**
 * @file RHIVertexLayout.hpp
 * @brief Defines vertex buffers and attributes.
 */

#pragma once

#include <RHIExport.hpp>
#include <Templates/Vector.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIFormat.hpp>
#include <Types/RHIHash.hpp>

namespace Lumen
{

/**
 * @enum ERHIVertexInputRate
 * @brief Per-vertex or per-instance stepping.
 */
enum class ERHIVertexInputRate : UInt8
{
    Vertex,
    Instance

};

/**
 * @struct FRHIVertexBufferLayout
 * @brief Binding slot, stride, and input rate.
 */
struct FRHIVertexBufferLayout
{
    UInt32 Slot              = 0U;
    UInt32 Stride            = 0U;
    ERHIVertexInputRate Rate = ERHIVertexInputRate::Vertex;
};

/**
 * @struct FRHIVertexAttribute
 * @brief Shader location, buffer slot, format, and byte offset.
 */
struct FRHIVertexAttribute
{
    UInt32 Location   = 0U;
    UInt32 BufferSlot = 0U;
    ERHIFormat Format = ERHIFormat::Unknown;
    UInt32 Offset     = 0U;
};

/**
 * @struct FRHIVertexLayout
 * @brief Canonical vertex input description.
 */
struct RHI_EXPORT FRHIVertexLayout
{
    TVector<FRHIVertexBufferLayout> Buffers;
    TVector<FRHIVertexAttribute> Attributes;
    /** @return Success for unique locations/slots and referenced buffers. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    /** @return Stable content hash. */
    [[nodiscard]] UInt64 GetHash () const noexcept;
};

} // namespace Lumen
