/**
 * @file OpenGLQueueLifetimeTests.cpp
 * @brief Functional coverage for condition-variable waits and loss wakeups in M4.3.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"

#include <HAL/LockGuard.hpp>
#include <HAL/PlatformMutex.hpp>
#include <HAL/PlatformTime.hpp>

#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Commands/RHICopyDescs.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>

#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

#include <limits>

namespace Lumen
{

namespace
{

    using namespace RHI;

    LUMEN_TEST( OpenGLQueueLifetime, WaitsWakeOnFenceCompletion )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );

        TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        LUMEN_CHECK_TRUE( Queue->WaitIdle().HasValue() );

        TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
        TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );

        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Completion   = { 1ULL };
        LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
        LUMEN_CHECK_TRUE( Queue->Wait( { 1ULL }, 5.0 ).HasValue() );
        LUMEN_CHECK_TRUE( Queue->GetCompletedValue().Value >= 1ULL );
        LUMEN_CHECK_TRUE( Queue->WaitIdle().HasValue() );
    }

    LUMEN_TEST( OpenGLQueueLifetime, WaitValidatesTimeoutAndPolls )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Copy, 0U );

        const FRHIExpected Negative = Queue->Wait( { 1ULL }, -1.0 );
        LUMEN_CHECK_HAS_ERROR( Negative );
        LUMEN_CHECK_EQ( Negative.Error().Code, ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_HAS_ERROR( Queue->Wait( { 1ULL }, std::numeric_limits<Float64>::infinity() ) );
        const FRHIExpected TimedOut = Queue->Wait( { 1ULL }, 0.0 );
        LUMEN_CHECK_HAS_ERROR( TimedOut );
        LUMEN_CHECK_EQ( TimedOut.Error().Code, ERHIErrorCode::Timeout );
        LUMEN_CHECK_TRUE( Queue->Wait( {}, 0.0 ).HasValue() );
    }

    LUMEN_TEST( OpenGLQueueLifetime, DeviceLossWakesBlockedWaiters )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );

        TAtomic<UInt32> Observed{ 0U };
        FThread Waiter( [&Device, &Queue, &Observed]
        {
            const FRHIExpected Result = Queue->Wait( { 99ULL }, 30.0 );
            if ( not Result )
            {
                Observed.store( Result.Error().Code == ERHIErrorCode::DeviceLost ? 2U : 3U );
                return;
            }
            Observed.store( 1U );
            ( void )Device;
        } );

        HAL::FPlatformTime::Sleep( 0.01 );
        LUMEN_CHECK_TRUE( RHI::Tests::GetRHIConformanceDeviceFactory().InjectDeviceLost( **Device ) );
        Waiter.Join();
        LUMEN_CHECK_EQ( Observed.load(), 2U );

        const FRHIExpected Rejected = Queue->Wait( { 100ULL }, 0.0 );
        LUMEN_CHECK_HAS_ERROR( Rejected );
        LUMEN_CHECK_EQ( Rejected.Error().Code, ERHIErrorCode::DeviceLost );
    }

    LUMEN_TEST( OpenGLQueueLifetime, ConcurrentSubmitWaitAndResourceDestruction )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Copy, 0U );

        TAtomic<UInt64> NextCompletion{ 1ULL };
        TAtomic<UInt32> Failures{ 0U };
        HAL::FPlatformMutex SubmitGate;

        const auto Worker = [&Device, &Queue, &NextCompletion, &Failures, &SubmitGate] () noexcept
        {
            for ( UInt32 Iteration = 0U; Iteration < 20U; ++Iteration )
            {
                const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Source =
                    ( *Device )->CreateBuffer( { .Size = 256ULL, .Usage = ERHIResourceUsage::CopySource } );
                const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Destination =
                    ( *Device )->CreateBuffer( { .Size = 256ULL, .Usage = ERHIResourceUsage::CopyDestination } );
                if ( not Source or not Destination )
                {
                    Failures.fetch_add( 1U );
                    return;
                }

                TSharedRef<IRHICommandAllocator> Allocator = *( *Device )->CreateCommandAllocator( ERHIQueueType::Copy );
                TSharedRef<IRHICommandList> List           = *( *Device )->CreateCommandList( Allocator );
                if ( not List->Begin().HasValue() or
                     not List->CopyBuffer( *Source, *Destination,
                                           FRHIBufferCopy{ .SourceOffset = 0ULL, .DestinationOffset = 0ULL, .Size = 256ULL } )
                             .HasValue() or
                     not List->End().HasValue() )
                {
                    Failures.fetch_add( 1U );
                    return;
                }

                UInt64 Completion = 0ULL;
                {
                    HAL::TLockGuard Lock( SubmitGate );
                    Completion = NextCompletion.fetch_add( 1ULL );
                    FRHISubmitDesc Submit;
                    Submit.CommandLists = { List };
                    Submit.Completion   = { Completion };
                    if ( not Queue->Submit( Submit ).HasValue() )
                    {
                        Failures.fetch_add( 1U );
                        return;
                    }
                }
                if ( not Queue->Wait( { Completion }, 5.0 ).HasValue() )
                {
                    Failures.fetch_add( 1U );
                    return;
                }
            }
        };

        FThread First( Worker );
        FThread Second( Worker );
        FThread Third( Worker );
        First.Join();
        Second.Join();
        Third.Join();
        LUMEN_CHECK_EQ( Failures.load(), 0U );
        LUMEN_CHECK_TRUE( Queue->WaitIdle().HasValue() );
        LUMEN_CHECK_TRUE( Queue->GetCompletedValue().Value >= 60ULL );
    }

} // namespace

} // namespace Lumen
