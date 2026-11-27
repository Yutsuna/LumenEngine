/**
 * @file RHIMultisampleState.hpp
 * @brief Defines multisample pipeline state.
 */

#pragma once

#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHISampleCount.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
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
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
