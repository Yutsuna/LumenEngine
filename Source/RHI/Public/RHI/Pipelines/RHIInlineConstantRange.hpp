/**
 * @file RHIInlineConstantRange.hpp
 * @brief Defines an inline constant range.
 */

#pragma once

#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIShaderStage.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHIInlineConstantRange
     * @brief Byte offset, size, and visible stages.
     */
    struct RHI_EXPORT FRHIInlineConstantRange
    {
        UInt32 Offset          = 0U;
        UInt32 Size            = 0U;
        ERHIShaderStage Stages = ERHIShaderStage::None;
        /** @return Success for nonzero size, stages, and aligned offset. */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };

    /**
     * @struct FRHIInlineConstantCommand
     * @brief Trivially copyable header preceding inline constant bytes in a command stream.
     */
    struct FRHIInlineConstantCommand
    {
        ERHIShaderStage Stages = ERHIShaderStage::None;
        UInt32 Offset          = 0U;
        UInt32 Size            = 0U;
    };

    /** @return Success when stages, alignment, size, and offset arithmetic define a valid update. */
    [[nodiscard]] RHI_EXPORT FRHIExpected ValidateRHIInlineConstantWrite ( ERHIShaderStage InStages,
                                                                           UInt32 InOffset,
                                                                           USize InSize ) noexcept;
} // namespace RHI

} // namespace Lumen
