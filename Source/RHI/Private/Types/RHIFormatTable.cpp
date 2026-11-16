/**
 * @file RHIFormatTable.cpp
 * @brief Implements the canonical format metadata table.
 */

#include "RHIFormatTable.hpp"

#include <Templates/Array.hpp>

namespace
{

Lumen::FRHIFormatInfo DescribeFormat ( const Lumen::ERHIFormat InFormat ) noexcept
{
    Lumen::FRHIFormatInfo Info{ .Format = InFormat, .Aspects = Lumen::ERHIFormatAspect::Color };

    switch ( InFormat )
    {
    case Lumen::ERHIFormat::Unknown:
        return Lumen::FRHIFormatInfo{};
    case Lumen::ERHIFormat::R8Unorm:
    case Lumen::ERHIFormat::R8Snorm:
    case Lumen::ERHIFormat::R8Uint:
    case Lumen::ERHIFormat::R8Sint:
        Info.BytesPerBlock = 1U;
        Info.ChannelCount  = 1U;
        break;
    case Lumen::ERHIFormat::RG8Unorm:
    case Lumen::ERHIFormat::RG8Snorm:
    case Lumen::ERHIFormat::RG8Uint:
    case Lumen::ERHIFormat::RG8Sint:
        Info.BytesPerBlock = 2U;
        Info.ChannelCount  = 2U;
        break;
    case Lumen::ERHIFormat::RGBA8Unorm:
    case Lumen::ERHIFormat::RGBA8Snorm:
    case Lumen::ERHIFormat::RGBA8Uint:
    case Lumen::ERHIFormat::RGBA8Sint:
    case Lumen::ERHIFormat::RGBA8Srgb:
    case Lumen::ERHIFormat::BGRA8Unorm:
    case Lumen::ERHIFormat::BGRA8Srgb:
        Info.BytesPerBlock = 4U;
        Info.ChannelCount  = 4U;
        break;
    case Lumen::ERHIFormat::R16Unorm:
    case Lumen::ERHIFormat::R16Snorm:
    case Lumen::ERHIFormat::R16Uint:
    case Lumen::ERHIFormat::R16Sint:
    case Lumen::ERHIFormat::R16Float:
        Info.BytesPerBlock = 2U;
        Info.ChannelCount  = 1U;
        break;
    case Lumen::ERHIFormat::RG16Unorm:
    case Lumen::ERHIFormat::RG16Snorm:
    case Lumen::ERHIFormat::RG16Uint:
    case Lumen::ERHIFormat::RG16Sint:
    case Lumen::ERHIFormat::RG16Float:
        Info.BytesPerBlock = 4U;
        Info.ChannelCount  = 2U;
        break;
    case Lumen::ERHIFormat::RGBA16Unorm:
    case Lumen::ERHIFormat::RGBA16Snorm:
    case Lumen::ERHIFormat::RGBA16Uint:
    case Lumen::ERHIFormat::RGBA16Sint:
    case Lumen::ERHIFormat::RGBA16Float:
        Info.BytesPerBlock = 8U;
        Info.ChannelCount  = 4U;
        break;
    case Lumen::ERHIFormat::R32Uint:
    case Lumen::ERHIFormat::R32Sint:
    case Lumen::ERHIFormat::R32Float:
        Info.BytesPerBlock = 4U;
        Info.ChannelCount  = 1U;
        break;
    case Lumen::ERHIFormat::RG32Uint:
    case Lumen::ERHIFormat::RG32Sint:
    case Lumen::ERHIFormat::RG32Float:
        Info.BytesPerBlock = 8U;
        Info.ChannelCount  = 2U;
        break;
    case Lumen::ERHIFormat::RGB32Uint:
    case Lumen::ERHIFormat::RGB32Sint:
    case Lumen::ERHIFormat::RGB32Float:
        Info.BytesPerBlock = 12U;
        Info.ChannelCount  = 3U;
        break;
    case Lumen::ERHIFormat::RGBA32Uint:
    case Lumen::ERHIFormat::RGBA32Sint:
    case Lumen::ERHIFormat::RGBA32Float:
        Info.BytesPerBlock = 16U;
        Info.ChannelCount  = 4U;
        break;
    case Lumen::ERHIFormat::D16Unorm:
        Info.BytesPerBlock = 2U;
        Info.ChannelCount  = 1U;
        Info.Aspects       = Lumen::ERHIFormatAspect::Depth;
        break;
    case Lumen::ERHIFormat::D24UnormS8Uint:
        Info.BytesPerBlock = 4U;
        Info.ChannelCount  = 2U;
        Info.Aspects       = Lumen::ERHIFormatAspect::Depth | Lumen::ERHIFormatAspect::Stencil;
        break;
    case Lumen::ERHIFormat::D32Float:
        Info.BytesPerBlock = 4U;
        Info.ChannelCount  = 1U;
        Info.Aspects       = Lumen::ERHIFormatAspect::Depth;
        break;
    case Lumen::ERHIFormat::D32FloatS8Uint:
        Info.BytesPerBlock = 8U;
        Info.ChannelCount  = 2U;
        Info.Aspects       = Lumen::ERHIFormatAspect::Depth | Lumen::ERHIFormatAspect::Stencil;
        break;
    case Lumen::ERHIFormat::BC1RGBAUnorm:
    case Lumen::ERHIFormat::BC1RGBASrgb:
        Info.BytesPerBlock = 8U;
        Info.BlockWidth    = 4U;
        Info.BlockHeight   = 4U;
        Info.ChannelCount  = 4U;
        Info.bIsCompressed = true;
        break;
    case Lumen::ERHIFormat::BC3RGBAUnorm:
    case Lumen::ERHIFormat::BC3RGBASrgb:
    case Lumen::ERHIFormat::BC7RGBAUnorm:
    case Lumen::ERHIFormat::BC7RGBASrgb:
        Info.BytesPerBlock = 16U;
        Info.BlockWidth    = 4U;
        Info.BlockHeight   = 4U;
        Info.ChannelCount  = 4U;
        Info.bIsCompressed = true;
        break;
    case Lumen::ERHIFormat::Count:
        return Lumen::FRHIFormatInfo{};
    }

    switch ( InFormat )
    {
    case Lumen::ERHIFormat::R8Snorm:
    case Lumen::ERHIFormat::R8Sint:
    case Lumen::ERHIFormat::RG8Snorm:
    case Lumen::ERHIFormat::RG8Sint:
    case Lumen::ERHIFormat::RGBA8Snorm:
    case Lumen::ERHIFormat::RGBA8Sint:
    case Lumen::ERHIFormat::R16Snorm:
    case Lumen::ERHIFormat::R16Sint:
    case Lumen::ERHIFormat::RG16Snorm:
    case Lumen::ERHIFormat::RG16Sint:
    case Lumen::ERHIFormat::RGBA16Snorm:
    case Lumen::ERHIFormat::RGBA16Sint:
    case Lumen::ERHIFormat::R32Sint:
    case Lumen::ERHIFormat::RG32Sint:
    case Lumen::ERHIFormat::RGB32Sint:
    case Lumen::ERHIFormat::RGBA32Sint:
        Info.bIsSigned = true;
        break;
    default:
        break;
    }

    switch ( InFormat )
    {
    case Lumen::ERHIFormat::R16Float:
    case Lumen::ERHIFormat::RG16Float:
    case Lumen::ERHIFormat::RGBA16Float:
    case Lumen::ERHIFormat::R32Float:
    case Lumen::ERHIFormat::RG32Float:
    case Lumen::ERHIFormat::RGB32Float:
    case Lumen::ERHIFormat::RGBA32Float:
    case Lumen::ERHIFormat::D32Float:
    case Lumen::ERHIFormat::D32FloatS8Uint:
        Info.bIsFloatingPoint = true;
        break;
    default:
        break;
    }

    Info.bIsSrgb = InFormat == Lumen::ERHIFormat::RGBA8Srgb or InFormat == Lumen::ERHIFormat::BGRA8Srgb or
                   InFormat == Lumen::ERHIFormat::BC1RGBASrgb or InFormat == Lumen::ERHIFormat::BC3RGBASrgb or
                   InFormat == Lumen::ERHIFormat::BC7RGBASrgb;
    return Info;
}

} // namespace

Lumen::TSpan<const Lumen::FRHIFormatInfo> Lumen::GetRHIFormatTable () noexcept
{
    static constexpr USize FormatCount                     = static_cast<USize>( Lumen::ERHIFormat::Count );
    static const TArray<FRHIFormatInfo, FormatCount> Table = []
    {
        TArray<FRHIFormatInfo, FormatCount> Result{};
        for ( USize Index = 0U; Index < FormatCount; ++Index )
        {
            Result[Index] = DescribeFormat( static_cast<ERHIFormat>( Index ) );
        }
        return Result;
    }();
    return { Table.data(), Table.size() };
}
