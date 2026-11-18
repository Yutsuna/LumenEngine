/**
 * @file RHIGraphicsPipelineDesc.hpp
 * @brief Defines dynamic-rendering graphics pipeline creation.
 */
#pragma once

#include <Pipelines/IRHIPipelineLayout.hpp>
#include <Pipelines/RHIBlendState.hpp>
#include <Pipelines/RHIDepthStencilState.hpp>
#include <Pipelines/RHIDynamicState.hpp>
#include <Pipelines/RHIMultisampleState.hpp>
#include <Pipelines/RHIPrimitiveTopology.hpp>
#include <Pipelines/RHIRasterizerState.hpp>
#include <Pipelines/RHIVertexLayout.hpp>

#include <Shaders/IRHIShader.hpp>

namespace Lumen
{

/**
 * @struct FRHIGraphicsPipelineDesc
 * @brief Complete portable graphics pipeline state.
 */
struct RHI_EXPORT FRHIGraphicsPipelineDesc
{
    TSharedPtr<IRHIShader> VertexShader;
    TSharedPtr<IRHIShader> PixelShader;
    TSharedPtr<IRHIPipelineLayout> Layout;
    FRHIVertexLayout VertexLayout;

    ERHIPrimitiveTopology Topology = ERHIPrimitiveTopology::TriangleList;

    FRHIBlendState Blend;
    FRHIDepthStencilState DepthStencil;
    FRHIRasterizerState Rasterizer;
    FRHIMultisampleState Multisample;

    ERHIDynamicState DynamicStates = ERHIDynamicState::None;
    TVector<ERHIFormat> ColorFormats;
    ERHIFormat DepthStencilFormat = ERHIFormat::Unknown;

    /** @return Success when required shaders/layout and all states are valid. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};

} // namespace Lumen
