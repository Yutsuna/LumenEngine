/**
 * @file RHIDeviceConformanceTests.cpp
 * @brief Runs the portable device contract on the linked backend.
 */

#include <Device/IRHIDevice.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIDeviceConformance, DevicePublishesNegotiatedContract )
{
    auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
    LUMEN_CHECK_TRUE( Device.HasValue() );
    LUMEN_CHECK_TRUE( ( *Device )->GetLimits().Validate().HasValue() );
    LUMEN_CHECK_TRUE( ( *Device )->GetCapabilities().Validate().HasValue() );
    LUMEN_CHECK_TRUE( ( *Device )->GetId().IsValid() );
    LUMEN_CHECK_TRUE( ( *Device )->WaitIdle().HasValue() );
}

} // namespace Lumen
