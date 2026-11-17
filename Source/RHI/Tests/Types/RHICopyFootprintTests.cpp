/**
 * @file RHICopyFootprintTests.cpp
 * @brief Tests copy footprint calculations.
 */

#include <Tests/UnitTest.hpp>
#include <Types/RHICopyFootprint.hpp>

namespace Lumen
{

LUMEN_TEST( RHICopyFootprint, ComputesAlignedAndCompressedLayouts )
{
    const TExpected<FRHICopyFootprint, FRHIError> Color =
        ComputeRHICopyFootprint( ERHIFormat::RGBA8Unorm, { 3U, 2U, 1U }, 0ULL, 16ULL );
    LUMEN_CHECK_TRUE( Color.HasValue() );
    LUMEN_CHECK_EQ( Color->RowPitch, 16ULL );
    LUMEN_CHECK_EQ( Color->TotalSize, 32ULL );

    const TExpected<FRHICopyFootprint, FRHIError> Compressed =
        ComputeRHICopyFootprint( ERHIFormat::BC7RGBAUnorm, { 9U, 9U, 1U } );
    LUMEN_CHECK_TRUE( Compressed.HasValue() );
    LUMEN_CHECK_EQ( Compressed->RowCount, 3U );
    LUMEN_CHECK_EQ( Compressed->RowPitch, 48ULL );
    LUMEN_CHECK_HAS_ERROR( ComputeRHICopyFootprint( ERHIFormat::Unknown, { 1U, 1U, 1U } ) );
}

} // namespace Lumen
