/**
 * @file RHIComputePipelineDesc.hpp
 * @brief Defines compute pipeline creation.
 */

#pragma once
#include <Pipelines/IRHIPipelineLayout.hpp>
#include <RHIExport.hpp>
#include <Shaders/IRHIShader.hpp>
namespace Lumen

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
} // namespace Lumen
