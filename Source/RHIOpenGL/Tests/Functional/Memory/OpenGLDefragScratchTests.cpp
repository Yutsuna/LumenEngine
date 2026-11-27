/**
 * @file OpenGLDefragScratchTests.cpp
 * @brief Functional coverage for the native defragmentation scratch predicate.
 */

#include <RHIOpenGL/Memory/OpenGLMemoryObjects.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    LUMEN_TEST( OpenGLDefragScratch, OverlappingRangesInSameBufferNeedScratch )
    {
        LUMEN_CHECK_TRUE( RHIOpenGL::NeedsScratchCopy( 7U, 7U, 256ULL, 512ULL, 0ULL, 512ULL ) );
    }

    LUMEN_TEST( OpenGLDefragScratch, DisjointRangesInSameBufferNeedNoScratch )
    {
        LUMEN_CHECK_FALSE( RHIOpenGL::NeedsScratchCopy( 7U, 7U, 2048ULL, 512ULL, 0ULL, 512ULL ) );
    }

    LUMEN_TEST( OpenGLDefragScratch, OverlappingOffsetsInDifferentBuffersNeedNoScratch )
    {
        LUMEN_CHECK_FALSE( RHIOpenGL::NeedsScratchCopy( 7U, 9U, 256ULL, 512ULL, 0ULL, 512ULL ) );
    }

    LUMEN_TEST( OpenGLDefragScratch, DisjointRangesInDifferentBuffersNeedNoScratch )
    {
        LUMEN_CHECK_FALSE( RHIOpenGL::NeedsScratchCopy( 7U, 9U, 2048ULL, 512ULL, 0ULL, 512ULL ) );
    }

    LUMEN_TEST( OpenGLDefragScratch, EmptyRangeNeedNoScratch )
    {
        LUMEN_CHECK_FALSE( RHIOpenGL::NeedsScratchCopy( 7U, 7U, 0ULL, 0ULL, 0ULL, 512ULL ) );
    }

} // namespace

} // namespace Lumen
