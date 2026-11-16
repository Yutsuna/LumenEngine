/**
 * @file RHIDynamicState.hpp
 * @brief Defines pipeline states supplied at recording time.
 */

#pragma once
#include <Core/EnumFlags.hpp>
namespace Lumen
{
/**
 * @enum ERHIDynamicState
 * @brief Dynamic viewport/scissor/blend/depth states.
 */
enum class ERHIDynamicState : UInt32
{
    None             = 0U,
    Viewport         = 1U << 0U,
    Scissor          = 1U << 1U,
    BlendConstants   = 1U << 2U,
    DepthBias        = 1U << 3U,
    StencilReference = 1U << 4U
};
LUMEN_ENUM_FLAGS( ERHIDynamicState );
} // namespace Lumen
