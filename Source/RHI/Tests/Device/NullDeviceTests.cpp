/**
 * @file NullDeviceTests.cpp
 * @brief Tests Null device status and fault propagation.
 */

#include <Device/NullDevice.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( NullDevice, PropagatesFaultsAndDeviceLoss )
{
    Tests::FNullDevice Device;
    LUMEN_CHECK_EQ( static_cast<UInt8>( Device.GetStatus() ), static_cast<UInt8>( ERHIDeviceStatus::Ready ) );
    LUMEN_CHECK_TRUE( Device.WaitIdle().HasValue() );
    Device.GetFaultPlan()->AddFailure( "WaitIdle", 1U,
                                       MakeRHIUnsupported( BackendFailure, ERHIErrorCondition::Device, "planned idle failure" ) );
    LUMEN_CHECK_HAS_ERROR( Device.WaitIdle() );
    Device.MarkLost();
    LUMEN_CHECK_HAS_ERROR( Device.WaitIdle() );
    LUMEN_CHECK_HAS_ERROR( Device.CreateBufferPool() );
}

} // namespace Lumen
