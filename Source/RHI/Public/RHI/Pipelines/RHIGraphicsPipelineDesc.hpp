/**
 * @file RHIGraphicsPipelineDesc.hpp
 * @brief Defines dynamic-rendering graphics pipeline creation.
 */
#pragma once

#include <RHI/Pipelines/IRHIPipelineLayout.hpp>
#include <RHI/Pipelines/RHIBlendState.hpp>
#include <RHI/Pipelines/RHIDepthStencilState.hpp>
#include <RHI/Pipelines/RHIDynamicState.hpp>
#include <RHI/Pipelines/RHIMultisampleState.hpp>
#include <RHI/Pipelines/RHIPrimitiveTopology.hpp>
#include <RHI/Pipelines/RHIRasterizerState.hpp>
#include <RHI/Pipelines/RHIVertexLayout.hpp>

#include <RHI/Shaders/IRHIShader.hpp>

namespace Lumen
{

namespace RHI
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

} // namespace RHI

} // namespace Lumen
