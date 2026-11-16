/**
 * @file RHIDeviceStressTests.cpp
 * @brief Stresses repeated Null device creation and destruction.
 */

#include <Device/NullDevice.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
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

} // namespace Lumen
