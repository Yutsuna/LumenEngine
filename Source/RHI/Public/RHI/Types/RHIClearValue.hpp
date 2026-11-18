/**
 * @file RHIClearValue.hpp
 * @brief Defines typed color and depth-stencil clear values.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Templates/Array.hpp>
#include <Templates/Variant.hpp>
#include <compare>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHIColorClearValue
     * @brief RGBA clear value in the attachment's numeric representation.
     */
    struct FRHIColorClearValue
    {
        TArray<Float32, 4U> Color = { 0.0F, 0.0F, 0.0F, 0.0F };
        friend constexpr auto operator<=>( const FRHIColorClearValue &, const FRHIColorClearValue & ) = default;
    };

    /**
     * @struct FRHIDepthStencilClearValue
     * @brief Depth and stencil clear components.
     */
    struct FRHIDepthStencilClearValue
    {
        Float32 Depth                                                                                               = 1.0F;
        UInt8 Stencil                                                                                               = 0U;
        friend constexpr auto operator<=>( const FRHIDepthStencilClearValue &, const FRHIDepthStencilClearValue & ) = default;
    };

    /** @using FRHIClearValue @brief Exactly one color or depth-stencil clear payload. */
    using FRHIClearValue = TVariant<FRHIColorClearValue, FRHIDepthStencilClearValue>;

} // namespace RHI

} // namespace Lumen
