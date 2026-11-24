/**
 * @file RHIBindingWrite.hpp
 * @brief Defines typed writes to binding sets.
 */

#pragma once

#include <Memory/SharedPtr.hpp>
#include <RHI/Bindings/RHIBindingType.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHIExport.hpp>
#include <Templates/Variant.hpp>

namespace Lumen
{

namespace RHI
{
    class IRHIBuffer;
    class IRHITextureView;
    class IRHISampler;

    /**
     * @struct FRHIBufferBinding
     * @brief Buffer range; size zero means the remainder.
     */
    struct FRHIBufferBinding
    {
        TSharedPtr<IRHIBuffer> Buffer;
        UInt64 Offset = 0ULL;
        UInt64 Size   = 0ULL;
    };

    /**
     * @struct FRHITextureBinding
     * @brief Texture view reference.
     */
    struct FRHITextureBinding
    {
        TSharedPtr<IRHITextureView> View;
    };

    /**
     * @struct FRHISamplerBinding
     * @brief Sampler reference.
     */
    struct FRHISamplerBinding
    {
        TSharedPtr<IRHISampler> Sampler;
    };

    /**
     * @struct FRHICombinedTextureSamplerBinding
     * @brief Texture view and sampler referenced by one combined descriptor.
     */
    struct FRHICombinedTextureSamplerBinding
    {
        TSharedPtr<IRHITextureView> View;
        TSharedPtr<IRHISampler> Sampler;
    };

    /** @using FRHIBindingResource @brief Exactly one typed bound resource. */
    using FRHIBindingResource =
        TVariant<FRHIBufferBinding, FRHITextureBinding, FRHISamplerBinding, FRHICombinedTextureSamplerBinding>;

    /**
     * @struct FRHIBindingWrite
     * @brief Destination slot, array element, and typed resource.
     */
    struct RHI_EXPORT FRHIBindingWrite
    {
        UInt32 Slot         = 0U;
        UInt32 ArrayElement = 0U;
        FRHIBindingResource Resource;

        /** @return Success when the active payload contains every required nonnull object. */
        [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    };
} // namespace RHI

} // namespace Lumen
