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
        [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    };
} // namespace RHI

} // namespace Lumen
