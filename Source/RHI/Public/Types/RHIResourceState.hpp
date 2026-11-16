/**
 * @file RHIResourceState.hpp
 * @brief Defines resource usage states and transition validation.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/EnumFlags.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

/** @enum ERHIResourceState @brief Resource access states used by barriers. */
enum class ERHIResourceState : UInt32
{
    Undefined          = 0U,
    Common             = 1U << 0U,
    VertexBuffer       = 1U << 1U,
    IndexBuffer        = 1U << 2U,
    ConstantBuffer     = 1U << 3U,
    ShaderResource     = 1U << 4U,
    UnorderedAccess    = 1U << 5U,
    RenderTarget       = 1U << 6U,
    DepthWrite         = 1U << 7U,
    DepthRead          = 1U << 8U,
    CopySource         = 1U << 9U,
    CopyDestination    = 1U << 10U,
    ResolveSource      = 1U << 11U,
    ResolveDestination = 1U << 12U,
    IndirectArgument   = 1U << 13U,
    Present            = 1U << 14U,
};

LUMEN_ENUM_FLAGS( ERHIResourceState );

/** @return True when a transition between these states has defined semantics. */
[[nodiscard]] RHI_EXPORT Bool IsValidRHIResourceTransition ( ERHIResourceState InBefore, ERHIResourceState InAfter ) noexcept;

/** @return True when a value contains no bit outside the public state mask. */
[[nodiscard]] RHI_EXPORT Bool IsValidRHIResourceState ( ERHIResourceState InState ) noexcept;

} // namespace Lumen
