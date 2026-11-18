/**
 * @file RHICommandListState.hpp
 * @brief Defines command-list lifecycle transitions.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @enum ERHICommandListState
     * @brief Lifecycle state of a recorded command list.
     */
    enum class ERHICommandListState : UInt8
    {
        Initial,
        Recording,
        Rendering,
        Executable,
        Pending
    };

    /** @return True when the lifecycle transition is legal. */
    [[nodiscard]] RHI_EXPORT Bool CanRHITransitionCommandList ( ERHICommandListState InFrom, ERHICommandListState InTo ) noexcept;

    /** @brief Applies a legal transition or returns InvalidState without modifying InOutState. */
    RHI_EXPORT TExpected<void, FRHIError> RHITransitionCommandList ( ERHICommandListState &InOutState,
                                                                     ERHICommandListState InTo ) noexcept;

} // namespace RHI

} // namespace Lumen
