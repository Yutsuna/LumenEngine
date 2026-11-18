/**
 * @file RHIFormatTable.cpp
 * @brief Implements the canonical format metadata table.
 */

#include "RHIFormatTable.hpp"

#include <Templates/Array.hpp>

namespace
{

Lumen::RHI::FRHIFormatInfo DescribeFormat ( const Lumen::RHI::ERHIFormat InFormat ) noexcept
{
    Lumen::RHI::FRHIFormatInfo Info{ .Format = InFormat, .Aspects = Lumen::RHI::ERHIFormatAspect::Color };

    switch ( InFormat )
    {
    case Lumen::RHI::ERHIFormat::Unknown:
        return Lumen::RHI::FRHIFormatInfo{};
    case Lumen::RHI::ERHIFormat::R8Unorm:
    case Lumen::RHI::ERHIFormat::R8Snorm:
    case Lumen::RHI::ERHIFormat::R8Uint:
    case Lumen::RHI::ERHIFormat::R8Sint:
        Info.BytesPerBlock = 1U;
        Info.ChannelCount  = 1U;
        break;
    case Lumen::RHI::ERHIFormat::RG8Unorm:
    case Lumen::RHI::ERHIFormat::RG8Snorm:
    case Lumen::RHI::ERHIFormat::RG8Uint:
    case Lumen::RHI::ERHIFormat::RG8Sint:
        Info.BytesPerBlock = 2U;
        Info.ChannelCount  = 2U;
        break;
    case Lumen::RHI::ERHIFormat::RGBA8Unorm:
    case Lumen::RHI::ERHIFormat::RGBA8Snorm:
    case Lumen::RHI::ERHIFormat::RGBA8Uint:
    case Lumen::RHI::ERHIFormat::RGBA8Sint:
    case Lumen::RHI::ERHIFormat::RGBA8Srgb:
    case Lumen::RHI::ERHIFormat::BGRA8Unorm:
    case Lumen::RHI::ERHIFormat::BGRA8Srgb:
        Info.BytesPerBlock = 4U;
        Info.ChannelCount  = 4U;
        break;
    case Lumen::RHI::ERHIFormat::R16Unorm:
    case Lumen::RHI::ERHIFormat::R16Snorm:
    case Lumen::RHI::ERHIFormat::R16Uint:
    case Lumen::RHI::ERHIFormat::R16Sint:
    case Lumen::RHI::ERHIFormat::R16Float:
        Info.BytesPerBlock = 2U;
        Info.ChannelCount  = 1U;
        break;
    case Lumen::RHI::ERHIFormat::RG16Unorm:
    case Lumen::RHI::ERHIFormat::RG16Snorm:
    case Lumen::RHI::ERHIFormat::RG16Uint:
    case Lumen::RHI::ERHIFormat::RG16Sint:
    case Lumen::RHI::ERHIFormat::RG16Float:
        Info.BytesPerBlock = 4U;
        Info.ChannelCount  = 2U;
        break;
    case Lumen::RHI::ERHIFormat::RGBA16Unorm:
    case Lumen::RHI::ERHIFormat::RGBA16Snorm:
    case Lumen::RHI::ERHIFormat::RGBA16Uint:
    case Lumen::RHI::ERHIFormat::RGBA16Sint:
    case Lumen::RHI::ERHIFormat::RGBA16Float:
        Info.BytesPerBlock = 8U;
        Info.ChannelCount  = 4U;
        break;
    case Lumen::RHI::ERHIFormat::R32Uint:
    case Lumen::RHI::ERHIFormat::R32Sint:
    case Lumen::RHI::ERHIFormat::R32Float:
        Info.BytesPerBlock = 4U;
        Info.ChannelCount  = 1U;
        break;
    case Lumen::RHI::ERHIFormat::RG32Uint:
    case Lumen::RHI::ERHIFormat::RG32Sint:
    case Lumen::RHI::ERHIFormat::RG32Float:
        Info.BytesPerBlock = 8U;
        Info.ChannelCount  = 2U;
        break;
    case Lumen::RHI::ERHIFormat::RGB32Uint:
    case Lumen::RHI::ERHIFormat::RGB32Sint:
    case Lumen::RHI::ERHIFormat::RGB32Float:
        Info.BytesPerBlock = 12U;
        Info.ChannelCount  = 3U;
        break;
    case Lumen::RHI::ERHIFormat::RGBA32Uint:
    case Lumen::RHI::ERHIFormat::RGBA32Sint:
    case Lumen::RHI::ERHIFormat::RGBA32Float:
        Info.BytesPerBlock = 16U;
        Info.ChannelCount  = 4U;
        break;
    case Lumen::RHI::ERHIFormat::D16Unorm:
        Info.BytesPerBlock = 2U;
        Info.ChannelCount  = 1U;
        Info.Aspects       = Lumen::RHI::ERHIFormatAspect::Depth;
        break;
    case Lumen::RHI::ERHIFormat::D24UnormS8Uint:
        Info.BytesPerBlock = 4U;
        Info.ChannelCount  = 2U;
        Info.Aspects       = Lumen::RHI::ERHIFormatAspect::Depth | Lumen::RHI::ERHIFormatAspect::Stencil;
        break;
    case Lumen::RHI::ERHIFormat::D32Float:
        Info.BytesPerBlock = 4U;
        Info.ChannelCount  = 1U;
        Info.Aspects       = Lumen::RHI::ERHIFormatAspect::Depth;
        break;
    case Lumen::RHI::ERHIFormat::D32FloatS8Uint:
        Info.BytesPerBlock = 8U;
        Info.ChannelCount  = 2U;
        Info.Aspects       = Lumen::RHI::ERHIFormatAspect::Depth | Lumen::RHI::ERHIFormatAspect::Stencil;
        break;
    case Lumen::RHI::ERHIFormat::BC1RGBAUnorm:
    case Lumen::RHI::ERHIFormat::BC1RGBASrgb:
        Info.BytesPerBlock = 8U;
        Info.BlockWidth    = 4U;
        Info.BlockHeight   = 4U;
        Info.ChannelCount  = 4U;
        Info.bIsCompressed = true;
        break;
    case Lumen::RHI::ERHIFormat::BC3RGBAUnorm:
    case Lumen::RHI::ERHIFormat::BC3RGBASrgb:
    case Lumen::RHI::ERHIFormat::BC7RGBAUnorm:
    case Lumen::RHI::ERHIFormat::BC7RGBASrgb:
        Info.BytesPerBlock = 16U;
        Info.BlockWidth    = 4U;
        Info.BlockHeight   = 4U;
        Info.ChannelCount  = 4U;
        Info.bIsCompressed = true;
        break;
    case Lumen::RHI::ERHIFormat::Count:
        return Lumen::RHI::FRHIFormatInfo{};
    }

    switch ( InFormat )
    {
    case Lumen::RHI::ERHIFormat::R8Snorm:
    case Lumen::RHI::ERHIFormat::R8Sint:
    case Lumen::RHI::ERHIFormat::RG8Snorm:
    case Lumen::RHI::ERHIFormat::RG8Sint:
    case Lumen::RHI::ERHIFormat::RGBA8Snorm:
    case Lumen::RHI::ERHIFormat::RGBA8Sint:
    case Lumen::RHI::ERHIFormat::R16Snorm:
    case Lumen::RHI::ERHIFormat::R16Sint:
    case Lumen::RHI::ERHIFormat::RG16Snorm:
    case Lumen::RHI::ERHIFormat::RG16Sint:
    case Lumen::RHI::ERHIFormat::RGBA16Snorm:
    case Lumen::RHI::ERHIFormat::RGBA16Sint:
    case Lumen::RHI::ERHIFormat::R32Sint:
    case Lumen::RHI::ERHIFormat::RG32Sint:
    case Lumen::RHI::ERHIFormat::RGB32Sint:
    case Lumen::RHI::ERHIFormat::RGBA32Sint:
        Info.bIsSigned = true;
        break;
    default:
        break;
    }

    switch ( InFormat )
    {
    case Lumen::RHI::ERHIFormat::R16Float:
    case Lumen::RHI::ERHIFormat::RG16Float:
    case Lumen::RHI::ERHIFormat::RGBA16Float:
    case Lumen::RHI::ERHIFormat::R32Float:
    case Lumen::RHI::ERHIFormat::RG32Float:
    case Lumen::RHI::ERHIFormat::RGB32Float:
    case Lumen::RHI::ERHIFormat::RGBA32Float:
    case Lumen::RHI::ERHIFormat::D32Float:
    case Lumen::RHI::ERHIFormat::D32FloatS8Uint:
        Info.bIsFloatingPoint = true;
        break;
    default:
        break;
    }

    Info.bIsSrgb = InFormat == Lumen::RHI::ERHIFormat::RGBA8Srgb or InFormat == Lumen::RHI::ERHIFormat::BGRA8Srgb or
                   InFormat == Lumen::RHI::ERHIFormat::BC1RGBASrgb or InFormat == Lumen::RHI::ERHIFormat::BC3RGBASrgb or
                   InFormat == Lumen::RHI::ERHIFormat::BC7RGBASrgb;
    return Info;
}

} // namespace

Lumen::TSpan<const Lumen::RHI::FRHIFormatInfo> Lumen::RHI::GetRHIFormatTable () noexcept
{
    static constexpr USize FormatCount                     = static_cast<USize>( Lumen::RHI::ERHIFormat::Count );
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
