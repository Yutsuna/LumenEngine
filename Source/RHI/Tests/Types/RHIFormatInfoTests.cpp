/**
 * @file RHIFormatInfoTests.cpp
 * @brief Tests format metadata invariants.
 */

#include <RHI/Types/RHIFormatInfo.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIFormatInfo, DescribesColorDepthAndCompression )
    {
        const FRHIFormatInfo &Color = GetRHIFormatInfo( ERHIFormat::RGBA8Srgb );
        LUMEN_CHECK_TRUE( Color.HasColor() and Color.bIsSrgb );
        LUMEN_CHECK_EQ( Color.BytesPerBlock, 4U );

        const FRHIFormatInfo &Depth = GetRHIFormatInfo( ERHIFormat::D24UnormS8Uint );
        LUMEN_CHECK_TRUE( Depth.HasDepth() and Depth.HasStencil() );

        const FRHIFormatInfo &Compressed = GetRHIFormatInfo( ERHIFormat::BC1RGBAUnorm );
        LUMEN_CHECK_TRUE( Compressed.bIsCompressed );
        LUMEN_CHECK_EQ( Compressed.BlockWidth, 4U );
    }

} // namespace RHI

} // namespace Lumen
