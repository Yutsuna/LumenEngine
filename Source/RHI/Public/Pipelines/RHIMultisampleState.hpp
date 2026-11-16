/**
 * @file RHIMultisampleState.hpp
 * @brief Defines multisample pipeline state.
 */

#pragma once
#include <RHIExport.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHISampleCount.hpp>
namespace Lumen

{
/**
 * @struct FRHIMultisampleState
 * @brief Sample count, mask, and alpha coverage options.
 */
struct RHI_EXPORT FRHIMultisampleState
{
    ERHISampleCount Count = ERHISampleCount::Count1;
    UInt32 SampleMask     = 0xFFFFFFFFU;
    Bool bAlphaToCoverage = false;
    /** @return Success for a supported power-of-two count and nonzero mask. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
