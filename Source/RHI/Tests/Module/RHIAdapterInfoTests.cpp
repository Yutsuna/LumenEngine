/**
 * @file RHIAdapterInfoTests.cpp
 * @brief Tests adapter information validation.
 */

#include <Device/NullDeviceDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIAdapterInfo, DefaultNullAdapterIsCoherent )
{
    const Tests::FNullDeviceDesc Desc;
    LUMEN_CHECK_TRUE( Desc.Adapter.Validate().HasValue() );
    FRHIAdapterInfo Invalid = Desc.Adapter;
    Invalid.QueueFamilies.clear();
    LUMEN_CHECK_HAS_ERROR( Invalid.Validate() );
}

} // namespace Lumen
