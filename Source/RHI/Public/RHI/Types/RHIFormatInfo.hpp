/**
 * @file RHIFormatInfo.hpp
 * @brief Defines metadata and lookup for portable formats.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/EnumFlags.hpp>
#include <RHI/Types/RHIFormat.hpp>
#include <RHIExport.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @enum ERHIFormatAspect
     * @brief Data planes present in a format.
     */
    enum class ERHIFormatAspect : UInt8
    {
        None    = 0U,
        Color   = 1U << 0U,
        Depth   = 1U << 1U,
        Stencil = 1U << 2U,
    };

    LUMEN_ENUM_FLAGS( ERHIFormatAspect );

    /**
     * @struct FRHIFormatInfo
     * @brief Size, channels, aspects, and compression metadata.
     */
    struct FRHIFormatInfo
    {
        ERHIFormat Format        = ERHIFormat::Unknown;
        UInt16 BytesPerBlock     = 0U;
        UInt8 BlockWidth         = 1U;
        UInt8 BlockHeight        = 1U;
        UInt8 ChannelCount       = 0U;
        ERHIFormatAspect Aspects = ERHIFormatAspect::None;
        Bool bIsCompressed       = false;
        Bool bIsSrgb             = false;
        Bool bIsSigned           = false;
        Bool bIsFloatingPoint    = false;

        /** @return True when color data is present. */
        [[nodiscard]] constexpr Bool HasColor () const noexcept
        {
            return HasAnyFlags( Aspects, ERHIFormatAspect::Color );
        }
        /** @return True when depth data is present. */
        [[nodiscard]] constexpr Bool HasDepth () const noexcept
        {
            return HasAnyFlags( Aspects, ERHIFormatAspect::Depth );
        }
        /** @return True when stencil data is present. */
        [[nodiscard]] constexpr Bool HasStencil () const noexcept
        {
            return HasAnyFlags( Aspects, ERHIFormatAspect::Stencil );
        }
    };

    /** @return Metadata for every enum value, including Unknown. */
    [[nodiscard]] RHI_EXPORT const FRHIFormatInfo &GetRHIFormatInfo ( ERHIFormat InFormat ) noexcept;

    /** @return Empty when InFormat is outside the enum range, otherwise its metadata. */
    [[nodiscard]] RHI_EXPORT TOptional<FRHIFormatInfo> TryGetRHIFormatInfo ( ERHIFormat InFormat ) noexcept;

} // namespace RHI

} // namespace Lumen
