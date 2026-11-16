/**
 * @file RHISynchronizationConformanceTests.cpp
 * @brief Runs portable semaphore and deferred lifetime scenarios.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"
#include <Device/IRHIDevice.hpp>
#include <Synchronization/IRHISemaphore.hpp>
#include <Synchronization/RHIDeferredReleaseQueue.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHISynchronizationConformance, TimelineWaitGatesDeferredLifetime )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        auto Semaphore = *( *Device )->CreateSemaphore( {} );
        FRHIDeferredReleaseQueue Releases;
        UInt32 Destroyed              = 0U;
        const auto IncrementDestroyed = [&Destroyed]
        {
            ++Destroyed;
        };
        const auto Enqueued = Releases.Enqueue( { 3ULL }, IncrementDestroyed );
        LUMEN_CHECK_TRUE( Enqueued.HasValue() );
        LUMEN_CHECK_EQ( Releases.Poll( { Semaphore->GetValue() } ), 0U );
        LUMEN_CHECK_TRUE( Semaphore->Signal( 3ULL ).HasValue() );
        LUMEN_CHECK_TRUE( Semaphore->Wait( 3ULL, 0.0 ).HasValue() );
        LUMEN_CHECK_EQ( Releases.Poll( { Semaphore->GetValue() } ), 1U );
        LUMEN_CHECK_EQ( Destroyed, 1U );
    }

} // namespace
} // namespace Lumen
