/**
 * @file RHIValidationDeviceTests.cpp
 * @brief Tests device decoration, queue identity, status, and backend error preservation.
 */
#include "Validation/RHIValidationTestUtils.hpp"
#include <Queue/IRHIQueue.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIValidationDevice, PreservesCapabilitiesIdentityAndQueueWrappers )
    {
        Tests::FRecordingDebugSink Sink;
        TSharedRef<IRHIDevice> Device   = Tests::Validation::MakeDevice( Sink );
        TSharedRef<IRHIQueue> Queue     = *Device->GetQueue( ERHIQueueType::Graphics, 0U );
        TSharedRef<IRHIQueue> SameQueue = *Device->GetQueue( ERHIQueueType::Graphics, 0U );
        LUMEN_CHECK_EQ( Queue.Get(), SameQueue.Get() );
        LUMEN_CHECK_EQ( Queue->GetType(), ERHIQueueType::Graphics );
        LUMEN_CHECK_TRUE( Device->GetLimits().MaxBufferSize > 0ULL );
        LUMEN_CHECK_TRUE( Device->WaitIdle().HasValue() );
        Device->SetDebugName( "Validated" );
        LUMEN_CHECK_TRUE( Device->GetDebugName() == "Validated" );
        TSharedRef<IRHIBuffer> Buffer = Tests::Validation::MakeBuffer( Device );
        LUMEN_CHECK_TRUE( UnwrapRHI( Buffer.Get() ) != Buffer.Get() );
        LUMEN_CHECK_EQ( Buffer->GetDesc().Size, 64ULL );
    }

    LUMEN_TEST( RHIValidationDevice, PropagatesBackendFailuresAndLostStatus )
    {
        Tests::FNullDeviceDesc NullDesc;
        const FRHIError Injected( ERHIErrorCode::BackendFailure, ERHIErrorCondition::Resource, "injected" );
        NullDesc.FaultPlan->AddFailure( "CreateBuffer", 1U, Injected );
        TSharedRef<Tests::FNullDevice> Null = MakeShared<Tests::FNullDevice>( NullDesc );
        TSharedRef<IRHIDevice> Backend      = Null;
        TSharedRef<IRHIDevice> Device       = *WrapRHIDevice( Backend );
        FRHIBufferDesc BufferDesc;
        BufferDesc.Size                                     = 64ULL;
        BufferDesc.Usage                                    = ERHIResourceUsage::ShaderResource;
        TExpected<TSharedRef<IRHIBuffer>, FRHIError> Failed = Device->CreateBuffer( BufferDesc );
        LUMEN_CHECK_HAS_ERROR( Failed );
        LUMEN_CHECK_EQ( Failed.Error().Code, ERHIErrorCode::BackendFailure );
        LUMEN_CHECK_TRUE( Failed.Error().Message == "injected" );
        Null->MarkLost();
        LUMEN_CHECK_EQ( Device->GetStatus(), ERHIDeviceStatus::Lost );
        LUMEN_CHECK_EQ( Device->CreateBuffer( BufferDesc ).Error().Code, ERHIErrorCode::DeviceLost );
    }
} // namespace
} // namespace Lumen
