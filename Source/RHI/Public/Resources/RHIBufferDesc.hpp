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
 */
struct RHI_EXPORT FRHIBufferDesc
{
    UInt64 Size                    = 0ULL;
    ERHIResourceUsage Usage        = ERHIResourceUsage::None;
    ERHIMemoryClass MemoryClass    = ERHIMemoryClass::DeviceLocal;
    ERHIResourceState InitialState = ERHIResourceState::Common;
    /** @return Success for nonzero size and known usage/state. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
