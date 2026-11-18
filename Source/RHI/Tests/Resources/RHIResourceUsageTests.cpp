/**
 * @file RHIResourceUsageTests.cpp
 * @brief Tests known resource usage masks.
 */

#include <Resources/RHIResourceUsage.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIResourceUsage, AcceptsCombinedKnownFlags )
{
    const ERHIResourceUsage Usage = ERHIResourceUsage::CopySource | ERHIResourceUsage::ShaderResource;
    LUMEN_CHECK_TRUE( IsValidRHIResourceUsage( Usage ) );
    LUMEN_CHECK_TRUE( HasAnyFlags( Usage, ERHIResourceUsage::CopySource ) );
    LUMEN_CHECK_FALSE( IsValidRHIResourceUsage( static_cast<ERHIResourceUsage>( 1U << 30U ) ) );
}

} // namespace Lumen
