/**
 * @file RHIResourceUsage.hpp
 * @brief Defines legal buffer and texture usage flags.
 */
#pragma once
#include <Core/EnumFlags.hpp>
#include <RHIExport.hpp>
namespace Lumen
{
/** @enum ERHIResourceUsage @brief Operations a resource may be used for. */
enum class ERHIResourceUsage : UInt32
{
    None             = 0U,
    CopySource       = 1U << 0U,
    CopyDestination  = 1U << 1U,
    VertexBuffer     = 1U << 2U,
    IndexBuffer      = 1U << 3U,
    ConstantBuffer   = 1U << 4U,
    ShaderResource   = 1U << 5U,
    UnorderedAccess  = 1U << 6U,
    RenderTarget     = 1U << 7U,
    DepthStencil     = 1U << 8U,
    IndirectArgument = 1U << 9U,
    QueryResolve     = 1U << 10U,
};
LUMEN_ENUM_FLAGS( ERHIResourceUsage );
/** @return True when no unknown usage bit is present. */
[[nodiscard]] RHI_EXPORT Bool IsValidRHIResourceUsage ( ERHIResourceUsage InUsage ) noexcept;
} // namespace Lumen
