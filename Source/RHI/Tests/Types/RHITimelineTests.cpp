/**
 * @file RHITimelineTests.cpp
 * @brief Tests timeline completion semantics.
 */
#include <Tests/UnitTest.hpp>
#include <Types/RHITimeline.hpp>

namespace Lumen
{

LUMEN_TEST( RHITimeline, CompletionIsMonotonic )
{
    LUMEN_CHECK_TRUE( IsRHICompletionReached( { 4ULL }, { 4ULL } ) );
    LUMEN_CHECK_TRUE( IsRHICompletionReached( { 5ULL }, { 4ULL } ) );
    LUMEN_CHECK_FALSE( IsRHICompletionReached( { 3ULL }, { 4ULL } ) );
    LUMEN_CHECK_TRUE( IsRHICompletionReached( {}, {} ) );
    LUMEN_CHECK_FALSE( FRHITimelineValue{}.IsValid() );
}

} // namespace Lumen
