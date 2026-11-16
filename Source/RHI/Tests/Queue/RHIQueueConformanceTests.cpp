/**
 * @file RHIQueueConformanceTests.cpp
 * @brief Runs portable queue signal and completion scenarios.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"
#include <Device/IRHIDevice.hpp>
#include <Queue/IRHIQueue.hpp>
#include <Synchronization/IRHISemaphore.hpp>
#include <Tests/UnitTest.hpp>
namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIQueueConformance, SubmitsSignalsAndWaitsForCompletion )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        auto Queue     = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        auto Semaphore = *( *Device )->CreateSemaphore( {} );
        FRHISubmitDesc Desc;
        Desc.Signals    = { { .Semaphore = Semaphore, .Value = 7ULL } };
        Desc.Completion = { 1ULL };
        LUMEN_CHECK_TRUE( Queue->Submit( Desc ).HasValue() );
        LUMEN_CHECK_EQ( Semaphore->GetValue(), 7ULL );
        LUMEN_CHECK_TRUE( Queue->WaitIdle().HasValue() );
        LUMEN_CHECK_EQ( Queue->GetCompletedValue().Value, 1ULL );
        LUMEN_CHECK_TRUE( Semaphore->Wait( 7ULL, 0.0 ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( ( *Device )->GetQueue( ERHIQueueType::Copy, 99U ) );
    }

} // namespace
} // namespace Lumen
