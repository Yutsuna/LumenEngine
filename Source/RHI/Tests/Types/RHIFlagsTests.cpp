/**
 * @file RHIFlagsTests.cpp
 * @brief Tests common RHI flag operations.
 */

#include <RHI/Types/RHIFlags.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIFlags, CombinesAndTestsFlags )
    {
        ERHIObjectFlags Flags = ERHIObjectFlags::Shared | ERHIObjectFlags::Transient;
        LUMEN_CHECK_TRUE( HasAnyFlags( Flags, ERHIObjectFlags::Shared ) );
        LUMEN_CHECK_FALSE( HasAnyFlags( Flags, ERHIObjectFlags::DisableDebugTracking ) );
        Flags &= ERHIObjectFlags::Transient;
        LUMEN_CHECK_TRUE( Flags == ERHIObjectFlags::Transient );
    }

} // namespace RHI

} // namespace Lumen
