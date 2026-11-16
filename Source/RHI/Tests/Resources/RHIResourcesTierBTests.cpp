/**
 * @file RHIResourcesTierBTests.cpp
 * @brief Tests mapping and device-loss resource paths.
 */

#include "Device/NullDevice.hpp"
#include <Resources/IRHIBuffer.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIResourcesTierB, RejectsDeviceLocalMapAndCreationAfterLoss )
{
    Tests::FNullDevice Device;
    auto Buffer = Device.CreateBuffer( { .Size = 16ULL, .Usage = ERHIResourceUsage::CopyDestination } );
    LUMEN_CHECK_HAS_ERROR( ( *Buffer )->Map( 0ULL, 1ULL ) );
    Device.MarkLost();
    LUMEN_CHECK_HAS_ERROR( Device.CreateBuffer( { .Size = 16ULL } ) );
}

} // namespace Lumen
