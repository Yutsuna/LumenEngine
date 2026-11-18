/**
 * @file RHIComputePipelineDesc.hpp
 * @brief Defines compute pipeline creation.
 */

#pragma once

#include <RHI/Pipelines/IRHIPipelineLayout.hpp>
#include <RHI/Shaders/IRHIShader.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHIComputePipelineDesc
     * @brief Compute shader and its layout.
     */
    struct RHI_EXPORT FRHIComputePipelineDesc
    {
        TSharedPtr<IRHIShader> Shader;
        TSharedPtr<IRHIPipelineLayout> Layout;
        /** @return Success for nonnull compute shader and layout. */
        [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    };
} // namespace RHI

} // namespace Lumen
