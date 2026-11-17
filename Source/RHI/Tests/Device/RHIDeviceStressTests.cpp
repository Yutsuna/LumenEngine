/**
 * @file RHIDeviceStressTests.cpp
 * @brief Stresses repeated devices, object registries, and scheduled failures.
 */

#include <Algo/Sort.hpp>
#include <Device/NullDevice.hpp>
#include <HAL/LockGuard.hpp>
#include <Resources/IRHIBuffer.hpp>
#include <Resources/RHIBufferDesc.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIDeviceStress, RepeatedDevicesHaveIndependentState )
    {
        for ( UInt32 Index = 0U; Index < 128U; ++Index )
        {
            auto Device = MakeShared<Tests::FNullDevice>();
            Device->SetDebugName( "StressDevice" );
            LUMEN_CHECK_TRUE( Device->GetDebugName() == "StressDevice" );
            LUMEN_CHECK_TRUE( Device->WaitIdle().HasValue() );
        }
    }

    LUMEN_TEST( RHIDeviceStress, FaultPlanOccurrencesFailOnlyTheirScheduledCall )
    {
        for ( UInt32 Occurrence = 1U; Occurrence <= 16U; ++Occurrence )
        {
            auto Plan = MakeShared<Tests::FNullFaultPlan>();
            Plan->AddFailure( "CreateBuffer", Occurrence,
                              FRHIError( ERHIErrorCode::BackendFailure, ERHIErrorCondition::Resource, "scheduled" ) );
            Tests::FNullDeviceDesc Desc;
            Desc.FaultPlan = Plan;
            Tests::FNullDevice Device( Desc );
            for ( UInt32 Call = 1U; Call <= 20U; ++Call )
            {
                auto Buffer = Device.CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource } );
                if ( Call == Occurrence )
                {
                    LUMEN_CHECK_HAS_ERROR( Buffer );
                    LUMEN_CHECK_EQ( Buffer.Error().Code, ERHIErrorCode::BackendFailure );
                    LUMEN_CHECK_EQ( Buffer.Error().Condition, ERHIErrorCondition::Resource );
                }
                else
                {
                    LUMEN_CHECK_TRUE( Buffer.HasValue() );
                }
            }
            LUMEN_CHECK_EQ( Device.GetStatus(), ERHIDeviceStatus::Ready );
            LUMEN_CHECK_TRUE( Device.WaitIdle().HasValue() );
        }
    }

    LUMEN_TEST( RHIDeviceStress, ConcurrentChildrenHaveUniqueIdsAndBalancedRegistry )
    {
        Tests::FNullDevice Device;
        const USize InitialTrackedCount = Device.GetTrackedObjectCount();
        TAtomic<UInt32> Failures{ 0U };
        TVector<UInt64> Identifiers;
        HAL::FPlatformRecursiveSharedMutex IdentifiersMutex;
        TVector<FThread> Workers;
        for ( UInt32 Worker = 0U; Worker < 4U; ++Worker )
        {
            Workers.emplace_back( [&Device, &Failures, &Identifiers, &IdentifiersMutex, Worker]
            {
                for ( UInt32 Index = 0U; Index < 128U; ++Index )
                {
                    auto Buffer = Device.CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource } );
                    if ( not Buffer )
                    {
                        Failures.fetch_add( 1U );
                        continue;
                    }
                    ( *Buffer )->SetDebugName( "ConcurrentBuffer" );
                    {
                        HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( IdentifiersMutex );
                        Identifiers.push_back( ( *Buffer )->GetId().GetValue() );
                    }
                }
            } );
        }
        for ( auto &Worker : Workers )
        {
            Worker.Join();
        }
        LUMEN_CHECK_EQ( Failures.load(), 0U );
        LUMEN_CHECK_EQ( Identifiers.size(), 512U );
        Algo::Sort( Identifiers );
        for ( USize Index = 1U; Index < Identifiers.size(); ++Index )
        {
            LUMEN_CHECK_NOT_EQ( Identifiers[Index - 1U], Identifiers[Index] );
        }
        LUMEN_CHECK_EQ( Device.GetTrackedObjectCount(), InitialTrackedCount );
    }

    LUMEN_TEST( RHIDeviceStress, ConcurrentCreationObservesDeviceLossCleanly )
    {
        Tests::FNullDevice Device;
        TAtomic<UInt32> Failures{ 0U };
        TVector<FThread> Workers;
        for ( UInt32 Worker = 0U; Worker < 4U; ++Worker )
        {
            Workers.emplace_back( [&Device, &Failures]
            {
                for ( UInt32 Index = 0U; Index < 256U; ++Index )
                {
                    auto Buffer = Device.CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource } );
                    if ( not Buffer and Buffer.Error().Code != ERHIErrorCode::DeviceLost )
                    {
                        Failures.fetch_add( 1U );
                    }
                }
            } );
        }
        Device.MarkLost();
        for ( auto &Worker : Workers )
        {
            Worker.Join();
        }
        LUMEN_CHECK_EQ( Failures.load(), 0U );
        LUMEN_CHECK_EQ( Device.GetStatus(), ERHIDeviceStatus::Lost );
        LUMEN_CHECK_HAS_ERROR( Device.CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource } ) );
    }

} // namespace
} // namespace Lumen
