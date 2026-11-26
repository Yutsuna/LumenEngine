/**
 * @file OpenGLFormatMapping.inl
 * @brief Inline implementation of OpenGLFormatMapping.hpp.
 */

#pragma once

#include <RHIOpenGL/Types/OpenGLFormatMapping.hpp>

template <typename MappingType> constexpr Lumen::TOptional<MappingType> MakeOpenGLFormatMapping ( MappingType InMapping ) noexcept
{
    return Lumen::TOptional<MappingType>( static_cast<MappingType &&>( InMapping ) );
}

constexpr Lumen::TOptional<Lumen::RHIOpenGL::FOpenGLFormatMapping>
Lumen::RHIOpenGL::MapOpenGLFormat ( const RHI::ERHIFormat InFormat ) noexcept
{
    using RHI::ERHIFormat;
    switch ( InFormat )
    {
    case ERHIFormat::R8Unorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8229U, 0x1903U, 0x1401U, 1 } );
    case ERHIFormat::R8Snorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8F94U, 0x1903U, 0x1400U, 1 } );
    case ERHIFormat::R8Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8232U, 0x1903U, 0x1405U, 1 } );
    case ERHIFormat::R8Sint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8231U, 0x1903U, 0x1404U, 1 } );
    case ERHIFormat::RG8Unorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x822BU, 0x8227U, 0x1401U, 2 } );
    case ERHIFormat::RG8Snorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8F95U, 0x8227U, 0x1400U, 2 } );
    case ERHIFormat::RG8Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8238U, 0x8227U, 0x1405U, 2 } );
    case ERHIFormat::RG8Sint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8237U, 0x8227U, 0x1404U, 2 } );
    case ERHIFormat::RGBA8Unorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ GlRgba8, GlRgba, GlUnsignedByte, 4 } );
    case ERHIFormat::RGBA8Snorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8F97U, GlRgba, 0x1400U, 4 } );
    case ERHIFormat::RGBA8Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8D20U, GlRgba, 0x1405U, 4 } );
    case ERHIFormat::RGBA8Sint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8D1AU, GlRgba, 0x1404U, 4 } );
    case ERHIFormat::RGBA8Srgb:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ GlSrgb8Alpha8, GlRgba, GlUnsignedByte, 4 } );
    case ERHIFormat::BGRA8Unorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ GlRgba8, 0x80E1U, GlUnsignedByte, 4 } );
    case ERHIFormat::BGRA8Srgb:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ GlSrgb8Alpha8, 0x80E1U, GlUnsignedByte, 4 } );
    case ERHIFormat::R16Unorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x822AU, 0x1903U, 0x1403U, 1 } );
    case ERHIFormat::R16Snorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8F98U, 0x1903U, 0x1402U, 1 } );
    case ERHIFormat::R16Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8234U, 0x1903U, 0x1405U, 1 } );
    case ERHIFormat::R16Sint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8233U, 0x1903U, 0x1404U, 1 } );
    case ERHIFormat::R16Float:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x822DU, 0x1903U, 0x1406U, 1 } );
    case ERHIFormat::RG16Unorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x822CU, 0x8227U, 0x1403U, 2 } );
    case ERHIFormat::RG16Snorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8F99U, 0x8227U, 0x1402U, 2 } );
    case ERHIFormat::RG16Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x823AU, 0x8227U, 0x1405U, 2 } );
    case ERHIFormat::RG16Sint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8239U, 0x8227U, 0x1404U, 2 } );
    case ERHIFormat::RG16Float:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x822FU, 0x8227U, 0x1406U, 2 } );
    case ERHIFormat::RGBA16Unorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x805BU, GlRgba, 0x1403U, 4 } );
    case ERHIFormat::RGBA16Snorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8F9AU, GlRgba, 0x1402U, 4 } );
    case ERHIFormat::RGBA16Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8D21U, GlRgba, 0x1405U, 4 } );
    case ERHIFormat::RGBA16Sint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8D1CU, GlRgba, 0x1404U, 4 } );
    case ERHIFormat::RGBA16Float:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x881AU, GlRgba, 0x1406U, 4 } );
    case ERHIFormat::R32Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8236U, 0x1903U, 0x1405U, 1 } );
    case ERHIFormat::R32Sint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8235U, 0x1903U, 0x1404U, 1 } );
    case ERHIFormat::R32Float:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x822EU, 0x1903U, 0x1406U, 1 } );
    case ERHIFormat::RG32Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x823CU, 0x8227U, 0x1405U, 2 } );
    case ERHIFormat::RG32Sint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x823BU, 0x8227U, 0x1404U, 2 } );
    case ERHIFormat::RG32Float:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8230U, 0x8227U, 0x1406U, 2 } );
    case ERHIFormat::RGB32Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8D23U, 0x1907U, 0x1405U, 3 } );
    case ERHIFormat::RGB32Sint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8D1EU, 0x1907U, 0x1404U, 3 } );
    case ERHIFormat::RGB32Float:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8815U, 0x1907U, 0x1406U, 3 } );
    case ERHIFormat::RGBA32Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8D24U, GlRgba, 0x1405U, 4 } );
    case ERHIFormat::RGBA32Sint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8D1FU, GlRgba, 0x1404U, 4 } );
    case ERHIFormat::RGBA32Float:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8814U, GlRgba, 0x1406U, 4 } );
    case ERHIFormat::D16Unorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x81A5U, 0x1902U, 0x1403U, 1, false, true } );
    case ERHIFormat::D24UnormS8Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x88F0U, 0x84F9U, 0x84FAU, 2, false, true } );
    case ERHIFormat::D32Float:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8CACU, 0x1902U, 0x1406U, 1, false, true } );
    case ERHIFormat::D32FloatS8Uint:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8CADU, 0x84F9U, 0x8DADU, 2, false, true } );
    case ERHIFormat::BC1RGBAUnorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x83F1U, 0U, 0U, 4, true, false } );
    case ERHIFormat::BC1RGBASrgb:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x83F2U, 0U, 0U, 4, true, false } );
    case ERHIFormat::BC3RGBAUnorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x83F3U, 0U, 0U, 4, true, false } );
    case ERHIFormat::BC3RGBASrgb:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x83F5U, 0U, 0U, 4, true, false } );
    case ERHIFormat::BC7RGBAUnorm:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8E8CU, 0U, 0U, 4, true, false } );
    case ERHIFormat::BC7RGBASrgb:
        return MakeOpenGLFormatMapping( FOpenGLFormatMapping{ 0x8E8DU, 0U, 0U, 4, true, false } );
    default:
        return {};
    }
}
