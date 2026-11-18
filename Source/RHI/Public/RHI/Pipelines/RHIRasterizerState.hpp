/**
 * @file RHIRasterizerState.hpp
 * @brief Defines rasterization state.
 */

#pragma once

#include <RHI/Types/RHIError.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @enum ERHIFillMode
     * @brief Polygon fill modes.
     */
    enum class ERHIFillMode : UInt8
    {
        Solid,
        Wireframe,
        Point
    };

    /**
     * @enum ERHICullMode
     * @brief Face culling modes.
     */
    enum class ERHICullMode : UInt8
    {
        None,
        Front,
        Back
    };

    /**
     * @enum ERHIFrontFace
     * @brief Winding defining the front face.
     */
    enum class ERHIFrontFace : UInt8
    {
        CounterClockwise,
        Clockwise

    };

    /**
     * @struct FRHIRasterizerState
     * @brief Fill, cull, winding, clipping, and depth bias.
     */
    struct RHI_EXPORT FRHIRasterizerState
    {
        ERHIFillMode Fill         = ERHIFillMode::Solid;
        ERHICullMode Cull         = ERHICullMode::Back;
        ERHIFrontFace FrontFace   = ERHIFrontFace::CounterClockwise;
        Bool bDepthClip           = true;
        Float32 DepthBiasConstant = 0.0F;
        Float32 DepthBiasSlope    = 0.0F;

        /** @return Success for defined enum values and finite bias. */
        [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
