/**
 * @file RHIBufferDesc.hpp
 * @brief Defines buffer creation parameters.
 */
#pragma once
#include <Memory/RHIMemoryClass.hpp>
#include <RHIExport.hpp>
#include <Resources/RHIResourceUsage.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIResourceState.hpp>
namespace Lumen

{
/**
 * @struct FRHIBufferDesc
 * @brief Size, usage, memory class, and initial state.
 * @details Immutable creation parameters for a buffer resource. The descriptor
 *          owns no memory and may be discarded once creation succeeds; callers
 *          must validate it before passing it to a device factory.
 */
struct RHI_EXPORT FRHIBufferDesc
{
    /** @brief Byte length of the buffer; must be nonzero. */
    UInt64 Size = 0ULL;
    /** @brief Bitmask of legal operations: copy, bind, transition and rendering. */
    ERHIResourceUsage Usage = ERHIResourceUsage::None;
    /** @brief Memory class requested for the backing allocation. */
    ERHIMemoryClass MemoryClass = ERHIMemoryClass::DeviceLocal;
    /** @brief State the buffer starts in after creation. */
    ERHIResourceState InitialState = ERHIResourceState::Common;

    /**
     * @brief Checks the descriptor without allocating or mutating any resource.
     * @return Success when Size is nonzero and Usage and InitialState are known
     *         values; otherwise InvalidArgument with the Resource condition.
     */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
