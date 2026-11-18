/**
 * @file RHIFlagsTests.cpp
 * @brief Tests common RHI flag operations.
 */

#include <Tests/UnitTest.hpp>
#include <Types/RHIFlags.hpp>

namespace Lumen
{

LUMEN_TEST( RHIFlags, CombinesAndTestsFlags )
{
    ERHIObjectFlags Flags = ERHIObjectFlags::Shared | ERHIObjectFlags::Transient;
    LUMEN_CHECK_TRUE( HasAnyFlags( Flags, ERHIObjectFlags::Shared ) );
    LUMEN_CHECK_FALSE( HasAnyFlags( Flags, ERHIObjectFlags::DisableDebugTracking ) );
    Flags &= ERHIObjectFlags::Transient;
    LUMEN_CHECK_TRUE( Flags == ERHIObjectFlags::Transient );
}

} // namespace Lumen
