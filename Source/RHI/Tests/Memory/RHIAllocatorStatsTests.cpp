/**
 * @file RHIAllocatorStatsTests.cpp
 * @brief Tests allocation statistics.
 */

#include <RHI/Memory/RHIAllocatorStats.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIAllocatorStats, TracksUsagePeakAndUtilization )
    {
        FRHIAllocatorStats Stats{ .Capacity = 100ULL };
        Stats.OnAllocate( 40ULL );
        Stats.OnAllocate( 20ULL );
        LUMEN_CHECK_EQ( Stats.Used, 60ULL );
        LUMEN_CHECK_EQ( Stats.Peak, 60ULL );
        Stats.OnFree( 20ULL );
        LUMEN_CHECK_EQ( Stats.Used, 40ULL );
        LUMEN_CHECK_EQ( Stats.Peak, 60ULL );
        LUMEN_CHECK_TRUE( Stats.GetUtilization() > 0.39 and Stats.GetUtilization() < 0.41 );
    }

} // namespace RHI

} // namespace Lumen
