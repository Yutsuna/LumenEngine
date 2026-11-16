/**
 * @file RHICommandStreamTests.cpp
 * @brief Tests aligned chunked command storage and retention.
 */

#include "Resources/NullResources.hpp"
#include <Commands/RHICommandStream.hpp>
#include <Tests/UnitTest.hpp>
namespace Lumen
{
namespace
{
    LUMEN_TEST( RHICommandStream, WritesReadsAndRecyclesChunks )
    {
        FRHICommandStream Stream( 64U );
        for ( UInt32 Index = 0U; Index < 40U; ++Index )
        {
            LUMEN_CHECK_TRUE( Stream.Write( 7U, Index ).HasValue() );
        }
        LUMEN_CHECK_EQ( Stream.GetCommandCount(), 40U );
        LUMEN_CHECK_TRUE( Stream.GetChunkCount() > 1U );
        auto Payload = Stream.ReadPayload<UInt32>( 39U, 7U );
        LUMEN_CHECK_TRUE( Payload.HasValue() );
        LUMEN_CHECK_EQ( *Payload, 39U );
        LUMEN_CHECK_HAS_ERROR( Stream.Read( 40U ) );
        LUMEN_CHECK_HAS_ERROR( Stream.ReadPayload<UInt64>( 0U, 7U ) );
        auto Object = MakeShared<Tests::FNullBuffer>( FRHIBufferDesc{ .Size = 16ULL, .Usage = ERHIResourceUsage::CopySource },
                                                      FRHIObjectId{ 1ULL } );
        Stream.Retain( StaticCastSharedRef<IRHIObject>( Object ) );
        LUMEN_CHECK_EQ( Stream.GetRetainedCount(), 1U );
        const USize AllocatedChunks = Stream.GetChunkCount();
        Stream.Clear();
        LUMEN_CHECK_EQ( Stream.GetCommandCount(), 0U );
        LUMEN_CHECK_EQ( Stream.GetRetainedCount(), 0U );
        LUMEN_CHECK_EQ( Stream.GetChunkCount(), AllocatedChunks );
        LUMEN_CHECK_TRUE( Stream.Write( 8U, 42U ).HasValue() );
    }

} // namespace
} // namespace Lumen
