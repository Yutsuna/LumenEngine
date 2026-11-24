/**
 * @file RHISamplerDesc.hpp
 * @brief Defines immutable sampler state.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHIExport.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHI
{
    /**
     * @enum ERHISamplerFilter
     * @brief Min/mag/mip filtering modes.
     */
    enum class ERHISamplerFilter : UInt8
    {
        Nearest,
        Linear
    };

    /**
     * @enum ERHISamplerAddressMode
     * @brief Coordinate wrapping modes.
     */
    enum class ERHISamplerAddressMode : UInt8
    {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder
    };

    /**
     * @enum ERHICompareFunction
     * @brief Depth and stencil comparison functions.
     */
    enum class ERHICompareFunction : UInt8
    {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always

    };

    /**
     * @struct FRHISamplerDesc
     * @brief Filtering, addressing, LOD, anisotropy, and comparison state.
     */
    struct RHI_EXPORT FRHISamplerDesc
    {
        ERHISamplerFilter MinFilter     = ERHISamplerFilter::Linear;
        ERHISamplerFilter MagFilter     = ERHISamplerFilter::Linear;
        ERHISamplerFilter MipFilter     = ERHISamplerFilter::Linear;
        ERHISamplerAddressMode AddressU = ERHISamplerAddressMode::Repeat;
        ERHISamplerAddressMode AddressV = ERHISamplerAddressMode::Repeat;
        ERHISamplerAddressMode AddressW = ERHISamplerAddressMode::Repeat;
        Float32 MinLod                  = 0.0F;
        Float32 MaxLod                  = 1000.0F;
        UInt32 MaxAnisotropy            = 1U;
        TOptional<ERHICompareFunction> CompareFunction;
        /** @return Success for ordered LOD and valid anisotropy. */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
