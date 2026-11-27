/**
 * @file OpenGLFormatMappingTests.cpp
 * @brief Checks the portable-format to GL-4.6 mapping table.
 */

#include <RHI/Types/RHIFormat.hpp>

#include <RHIOpenGL/Types/OpenGLFormatMapping.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    namespace
    {

        LUMEN_TEST( OpenGLFormatMapping, MapsCommonColorAndSrgbFormats )
        {
            const TOptional<FOpenGLFormatMapping> Rgba = MapOpenGLFormat( RHI::ERHIFormat::RGBA8Unorm );
            LUMEN_CHECK( Rgba );
            LUMEN_CHECK_EQ( Rgba->InternalFormat, GlRgba8 );
            LUMEN_CHECK_EQ( Rgba->ExternalFormat, GlRgba );
            LUMEN_CHECK_EQ( Rgba->Type, GlUnsignedByte );

            const TOptional<FOpenGLFormatMapping> Srgb = MapOpenGLFormat( RHI::ERHIFormat::RGBA8Srgb );
            LUMEN_CHECK( Srgb );
            LUMEN_CHECK_EQ( Srgb->InternalFormat, GlSrgb8Alpha8 );
            LUMEN_CHECK_EQ( Srgb->ExternalFormat, GlRgba );
        }

        LUMEN_TEST( OpenGLFormatMapping, PreservesBgraMemoryLayout )
        {
            const TOptional<FOpenGLFormatMapping> Bgra = MapOpenGLFormat( RHI::ERHIFormat::BGRA8Unorm );
            LUMEN_CHECK( Bgra );
            LUMEN_CHECK_EQ( Bgra->InternalFormat, GlRgba8 );
            LUMEN_CHECK_EQ( Bgra->ExternalFormat, 0x80E1U );
            LUMEN_CHECK_EQ( Bgra->Type, GlUnsignedByte );
        }

        LUMEN_TEST( OpenGLFormatMapping, RejectsUnmappedFormatsAndGivesDistinctCompressedFormats )
        {
            LUMEN_CHECK( not MapOpenGLFormat( RHI::ERHIFormat::Unknown ) );
            LUMEN_CHECK( not MapOpenGLFormat( RHI::ERHIFormat::Count ) );
            const TOptional<FOpenGLFormatMapping> Bc7Unorm = MapOpenGLFormat( RHI::ERHIFormat::BC7RGBAUnorm );
            const TOptional<FOpenGLFormatMapping> Bc7Srgb  = MapOpenGLFormat( RHI::ERHIFormat::BC7RGBASrgb );
            LUMEN_CHECK( Bc7Unorm );
            LUMEN_CHECK( Bc7Srgb );
            LUMEN_CHECK_NOT_EQ( Bc7Unorm->InternalFormat, Bc7Srgb->InternalFormat );
        }

    } // namespace

} // namespace RHIOpenGL

} // namespace Lumen
