/**
 * @file RHICopyDescs.hpp
 * @brief Defines buffer, texture, and linear footprint copies.
 */

#pragma once

#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIExtent.hpp>
#include <RHI/Types/RHIFormatInfo.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    struct RHI_EXPORT FRHIBufferCopy
    {
        UInt64 SourceOffset      = 0ULL;
        UInt64 DestinationOffset = 0ULL;
        UInt64 Size              = 0ULL;

        /**
         * @brief Validates the copy range without reference buffer sizes.
         * @return Success, or a structured FRHIError with the relevant condition.
         */
        [[nodiscard]] FRHIExpected Validate () const noexcept;

        /**
         * @brief Validates the copy against the source and destination sizes.
         * @param InSourceSize Size of the source buffer.
         * @param InDestinationSize Size of the destination buffer.
         * @param bSameBuffer True when both ranges belong to the same buffer.
         * @return Success, or a structured FRHIError with the relevant condition.
         */
        [[nodiscard]] FRHIExpected Validate ( UInt64 InSourceSize, UInt64 InDestinationSize, Bool bSameBuffer ) const noexcept;
    };
    struct RHI_EXPORT FRHITextureCopy
    {
        FRHIOffset3D SourceOffset;
        FRHIOffset3D DestinationOffset;
        FRHIExtent3D Extent;

        UInt32 SourceMip        = 0U;
        UInt32 SourceLayer      = 0U;
        UInt32 DestinationMip   = 0U;
        UInt32 DestinationLayer = 0U;

        ERHIFormatAspect Aspect = ERHIFormatAspect::Color;

        /**
         * @brief Validates the texture copy against a format.
         * @param InFormat Format whose aspect and footprint are checked.
         * @return Success, or a structured FRHIError with the relevant condition.
         */
        [[nodiscard]] FRHIExpected Validate ( ERHIFormat InFormat ) const noexcept;
    };

    /**
     * @struct FRHIBufferTextureCopy
     * @brief Describes a copy operation from a buffer to a texture.
     */
    struct RHI_EXPORT FRHIBufferTextureCopy
    {
        UInt64 BufferOffset = 0ULL;
        UInt64 RowPitch     = 0ULL;
        UInt64 SlicePitch   = 0ULL;

        FRHIOffset3D TextureOffset;
        FRHIExtent3D Extent;

        UInt32 Mip              = 0U;
        UInt32 Layer            = 0U;
        ERHIFormatAspect Aspect = ERHIFormatAspect::Color;

        /** @brief Zero pitches select a tightly packed footprint. */
        [[nodiscard]] FRHIExpected Validate ( ERHIFormat InFormat ) const noexcept;
    };
} // namespace RHI

} // namespace Lumen
