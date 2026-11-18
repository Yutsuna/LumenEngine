/**
 * @file RHITimelineTests.cpp
 * @brief Tests timeline completion semantics.
 */
#include <RHI/Types/RHITimeline.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHITimeline, CompletionIsMonotonic )
    {
        LUMEN_CHECK_TRUE( IsRHICompletionReached( { 4ULL }, { 4ULL } ) );
        LUMEN_CHECK_TRUE( IsRHICompletionReached( { 5ULL }, { 4ULL } ) );
        LUMEN_CHECK_FALSE( IsRHICompletionReached( { 3ULL }, { 4ULL } ) );
        LUMEN_CHECK_TRUE( IsRHICompletionReached( {}, {} ) );
        LUMEN_CHECK_FALSE( FRHITimelineValue{}.IsValid() );
    }

} // namespace RHI

} // namespace Lumen
