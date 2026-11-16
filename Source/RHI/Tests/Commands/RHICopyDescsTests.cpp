/**
 * @file RHICopyDescsTests.cpp
 * @brief Tests bounded copy descriptors and pitches.
 */

#include <Commands/RHICopyDescs.hpp>
#include <Tests/UnitTest.hpp>
#include <limits>
namespace Lumen
{
namespace
{
    LUMEN_TEST( RHICopyDescs, BufferCopiesCheckBoundsOverflowAndOverlap )
    {
        FRHIBufferCopy Copy{ .SourceOffset = 0ULL, .DestinationOffset = 32ULL, .Size = 16ULL };
        LUMEN_CHECK_TRUE( Copy.Validate( 64ULL, 64ULL, true ).HasValue() );
        Copy.DestinationOffset = 8ULL;
        LUMEN_CHECK_HAS_ERROR( Copy.Validate( 64ULL, 64ULL, true ) );
        LUMEN_CHECK_TRUE( Copy.Validate( 64ULL, 64ULL, false ).HasValue() );
        Copy.SourceOffset = std::numeric_limits<UInt64>::max() - 3ULL;
        Copy.Size         = 4ULL;
        LUMEN_CHECK_HAS_ERROR( Copy.Validate() );
        Copy.Size = 0ULL;
        LUMEN_CHECK_HAS_ERROR( Copy.Validate() );
    }
    LUMEN_TEST( RHICopyDescs, TextureCopiesCheckOffsetsExtentsAndAspects )
    {
        FRHITextureCopy Copy;
        Copy.Extent = { .Width = 4U, .Height = 4U, .Depth = 1U };
        LUMEN_CHECK_TRUE( Copy.Validate( ERHIFormat::RGBA8Unorm ).HasValue() );
        Copy.Aspect = ERHIFormatAspect::Depth;
        LUMEN_CHECK_HAS_ERROR( Copy.Validate( ERHIFormat::RGBA8Unorm ) );
        Copy.Aspect = ERHIFormatAspect::Depth | ERHIFormatAspect::Stencil;
        LUMEN_CHECK_TRUE( Copy.Validate( ERHIFormat::D24UnormS8Uint ).HasValue() );
        Copy.SourceOffset.X = -1;
        LUMEN_CHECK_HAS_ERROR( Copy.Validate( ERHIFormat::D24UnormS8Uint ) );
        Copy.SourceOffset.X = 0;
        Copy.Extent.Width   = 0U;
        LUMEN_CHECK_HAS_ERROR( Copy.Validate( ERHIFormat::D24UnormS8Uint ) );
    }
    LUMEN_TEST( RHICopyDescs, BufferTextureCopiesValidatePitchesAndSize )
    {
        FRHIBufferTextureCopy Copy;
        Copy.Extent = { .Width = 4U, .Height = 4U, .Depth = 1U };
        LUMEN_CHECK_TRUE( Copy.Validate( ERHIFormat::RGBA8Unorm ).HasValue() );
        Copy.RowPitch = 15ULL;
        LUMEN_CHECK_HAS_ERROR( Copy.Validate( ERHIFormat::RGBA8Unorm ) );
        Copy.RowPitch   = 16ULL;
        Copy.SlicePitch = 32ULL;
        LUMEN_CHECK_HAS_ERROR( Copy.Validate( ERHIFormat::RGBA8Unorm ) );
        Copy.SlicePitch = 64ULL;
        LUMEN_CHECK_TRUE( Copy.Validate( ERHIFormat::RGBA8Unorm ).HasValue() );
    }

} // namespace
} // namespace Lumen
