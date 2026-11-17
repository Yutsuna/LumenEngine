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
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TSharedRef<IRHISemaphore> Semaphore = *( *Device )->CreateSemaphore( {} );
        FRHIDeferredReleaseQueue Releases;
        UInt32 Destroyed                           = 0U;
        const TFunction<void()> IncrementDestroyed = [&Destroyed]
        {
            ++Destroyed;
        };
        const TExpected<void, FRHIError> Enqueued = Releases.Enqueue( { 3ULL }, IncrementDestroyed );
        LUMEN_CHECK_TRUE( Enqueued.HasValue() );
        LUMEN_CHECK_EQ( Releases.Poll( { Semaphore->GetValue() } ), 0U );
        LUMEN_CHECK_TRUE( Semaphore->Signal( 3ULL ).HasValue() );
        LUMEN_CHECK_TRUE( Semaphore->Wait( 3ULL, 0.0 ).HasValue() );
        LUMEN_CHECK_EQ( Releases.Poll( { Semaphore->GetValue() } ), 1U );
        LUMEN_CHECK_EQ( Destroyed, 1U );
    }

    LUMEN_TEST( RHISynchronizationConformance, BinaryAndTimelineSemaphoresKeepDistinctSemantics )
    {
        TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        FRHISemaphoreDesc BinaryDesc;
        BinaryDesc.Type                                        = ERHISemaphoreType::Binary;
        TExpected<TSharedRef<IRHISemaphore>, FRHIError> Binary = ( *Device )->CreateSemaphore( BinaryDesc );
        LUMEN_CHECK_TRUE( Binary.HasValue() );
        LUMEN_CHECK_TRUE( ( *Binary )->Signal( 1ULL ).HasValue() );
        LUMEN_CHECK_TRUE( ( *Binary )->Wait( 1ULL, 0.0 ).HasValue() );
        LUMEN_CHECK_EQ( ( *Binary )->GetValue(), 0ULL );
        const TExpected<void, FRHIError> BinaryTimeout = ( *Binary )->Wait( 1ULL, 0.0 );
        LUMEN_CHECK_HAS_ERROR( BinaryTimeout );
        LUMEN_CHECK_EQ( BinaryTimeout.Error().Code, ERHIErrorCode::Timeout );

        FRHISemaphoreDesc TimelineDesc;
        TimelineDesc.Type                                        = ERHISemaphoreType::Timeline;
        TExpected<TSharedRef<IRHISemaphore>, FRHIError> Timeline = ( *Device )->CreateSemaphore( TimelineDesc );
        if ( not( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::TimelineSemaphores ) )
        {
            LUMEN_CHECK_HAS_ERROR( Timeline );
            LUMEN_CHECK_EQ( Timeline.Error().Code, ERHIErrorCode::Unsupported );
            LUMEN_CHECK_EQ( Timeline.Error().Condition, ERHIErrorCondition::Capability );
            return;
        }
        LUMEN_CHECK_TRUE( Timeline.HasValue() );
        LUMEN_CHECK_TRUE( ( *Timeline )->Signal( 3ULL ).HasValue() );
        LUMEN_CHECK_TRUE( ( *Timeline )->Wait( 3ULL, 0.0 ).HasValue() );
        const TExpected<void, FRHIError> Regression = ( *Timeline )->Signal( 2ULL );
        LUMEN_CHECK_HAS_ERROR( Regression );
        LUMEN_CHECK_EQ( Regression.Error().Code, ERHIErrorCode::InvalidState );
        const TExpected<void, FRHIError> Timeout = ( *Timeline )->Wait( 4ULL, 0.0 );
        LUMEN_CHECK_HAS_ERROR( Timeout );
        LUMEN_CHECK_EQ( Timeout.Error().Code, ERHIErrorCode::Timeout );
    }

    LUMEN_TEST( RHISynchronizationConformance, DeferredShutdownDrainsAndRejectsNewEntries )
    {
        FRHIDeferredReleaseQueue Releases;
        UInt32 Destroyed                           = 0U;
        const TFunction<void()> IncrementDestroyed = [&Destroyed]
        {
            ++Destroyed;
        };
        LUMEN_CHECK_TRUE( Releases.Enqueue( { 99ULL }, IncrementDestroyed ).HasValue() );
        Releases.Shutdown();
        LUMEN_CHECK_TRUE( Releases.IsShutdown() );
        LUMEN_CHECK_EQ( Destroyed, 1U );
        LUMEN_CHECK_EQ( Releases.GetSize(), 0U );
        LUMEN_CHECK_EQ( Releases.Poll( { 99ULL } ), 0U );
        const TExpected<void, FRHIError> Enqueued = Releases.Enqueue( {}, IncrementDestroyed );
        LUMEN_CHECK_HAS_ERROR( Enqueued );
        LUMEN_CHECK_EQ( Enqueued.Error().Code, ERHIErrorCode::InvalidState );
        LUMEN_CHECK_EQ( Releases.Drain(), 0U );
    }

} // namespace
} // namespace Lumen
