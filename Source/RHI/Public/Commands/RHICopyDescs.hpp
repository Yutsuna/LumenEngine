/**
 * @file RHICopyDescs.hpp
 * @brief Defines buffer, texture, and linear footprint copies.
 */

#pragma once
#include <RHIExport.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIExtent.hpp>
#include <Types/RHIFormatInfo.hpp>
namespace Lumen
{
struct RHI_EXPORT FRHIBufferCopy
{
    UInt64 SourceOffset      = 0ULL;
    UInt64 DestinationOffset = 0ULL;
    UInt64 Size              = 0ULL;
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    [[nodiscard]] TExpected<void, FRHIError>
    Validate ( UInt64 InSourceSize, UInt64 InDestinationSize, Bool bSameBuffer ) const noexcept;
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
    [[nodiscard]] TExpected<void, FRHIError> Validate ( ERHIFormat InFormat ) const noexcept;
};
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
    [[nodiscard]] TExpected<void, FRHIError> Validate ( ERHIFormat InFormat ) const noexcept;
};
} // namespace Lumen
