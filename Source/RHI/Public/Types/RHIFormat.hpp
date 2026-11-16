/**
 * @file RHIFormat.hpp
 * @brief Defines the portable resource and vertex formats.
 */
#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

/** @enum ERHIFormat @brief Backend-independent texel and vertex formats. */
enum class ERHIFormat : UInt16
{
    Unknown = 0,
    R8Unorm,
    R8Snorm,
    R8Uint,
    R8Sint,
    RG8Unorm,
    RG8Snorm,
    RG8Uint,
    RG8Sint,
    RGBA8Unorm,
    RGBA8Snorm,
    RGBA8Uint,
    RGBA8Sint,
    RGBA8Srgb,
    BGRA8Unorm,
    BGRA8Srgb,
    R16Unorm,
    R16Snorm,
    R16Uint,
    R16Sint,
    R16Float,
    RG16Unorm,
    RG16Snorm,
    RG16Uint,
    RG16Sint,
    RG16Float,
    RGBA16Unorm,
    RGBA16Snorm,
    RGBA16Uint,
    RGBA16Sint,
    RGBA16Float,
    R32Uint,
    R32Sint,
    R32Float,
    RG32Uint,
    RG32Sint,
    RG32Float,
    RGB32Uint,
    RGB32Sint,
    RGB32Float,
    RGBA32Uint,
    RGBA32Sint,
    RGBA32Float,
    D16Unorm,
    D24UnormS8Uint,
    D32Float,
    D32FloatS8Uint,
    BC1RGBAUnorm,
    BC1RGBASrgb,
    BC3RGBAUnorm,
    BC3RGBASrgb,
    BC7RGBAUnorm,
    BC7RGBASrgb,
    Count
};

} // namespace Lumen
