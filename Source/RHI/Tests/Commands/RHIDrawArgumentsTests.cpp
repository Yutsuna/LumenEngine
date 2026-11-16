/**
 * @file RHIDrawArgumentsTests.cpp
 * @brief Tests draw, dispatch, and indirect arguments.
 */

#include <Commands/RHIDrawArguments.hpp>
#include <Tests/UnitTest.hpp>
namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIDrawArguments, RequireNonzeroDrawAndDispatchCounts )
    {
        FRHIDrawArguments Draw{ .VertexCount = 3U };
        LUMEN_CHECK_TRUE( Draw.Validate().HasValue() );
        Draw.InstanceCount = 0U;
        LUMEN_CHECK_HAS_ERROR( Draw.Validate() );
        FRHIDrawIndexedArguments Indexed{ .IndexCount = 3U };
        LUMEN_CHECK_TRUE( Indexed.Validate().HasValue() );
        Indexed.IndexCount = 0U;
        LUMEN_CHECK_HAS_ERROR( Indexed.Validate() );
        FRHIDispatchArguments Dispatch{ .GroupCountX = 2U, .GroupCountY = 3U, .GroupCountZ = 4U };
        LUMEN_CHECK_TRUE( Dispatch.Validate( 4U ).HasValue() );
        Dispatch.GroupCountY = 5U;
        LUMEN_CHECK_HAS_ERROR( Dispatch.Validate( 4U ) );
    }
    LUMEN_TEST( RHIDrawArguments, IndirectRangesCheckStrideAlignmentAndBufferBounds )
    {
        FRHIIndirectArguments Indirect{ .Offset = 16ULL, .Count = 2U, .Stride = 16U };
        LUMEN_CHECK_TRUE( Indirect.Validate( 64ULL, 16U ).HasValue() );
        Indirect.Stride = 18U;
        LUMEN_CHECK_HAS_ERROR( Indirect.Validate( 64ULL, 16U ) );
        Indirect.Stride = 16U;
        Indirect.Offset = 60ULL;
        LUMEN_CHECK_HAS_ERROR( Indirect.Validate( 64ULL, 16U ) );
        Indirect.Offset = 16ULL;
        Indirect.Count  = 0U;
        LUMEN_CHECK_HAS_ERROR( Indirect.Validate( 64ULL, 16U ) );
    }

} // namespace
} // namespace Lumen
