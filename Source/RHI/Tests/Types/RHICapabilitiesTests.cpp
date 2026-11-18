/**
 * @file RHICapabilitiesTests.cpp
 * @brief Tests feature-mask queries and validation.
 */

#include <Tests/UnitTest.hpp>
#include <Types/RHICapabilities.hpp>

namespace Lumen
{

LUMEN_TEST( RHICapabilities, SupportsRequiredFeatureSet )
{
    FRHICapabilities Capabilities{ .Features = ERHICapabilityFlags::DynamicRendering | ERHICapabilityFlags::IndirectDraw };
    LUMEN_CHECK_TRUE( Capabilities.Supports( ERHICapabilityFlags::DynamicRendering ) );
    LUMEN_CHECK_FALSE( Capabilities.Supports( ERHICapabilityFlags::RayTracing ) );
    LUMEN_CHECK_TRUE( Capabilities.Validate().HasValue() );
    Capabilities.Features = static_cast<ERHICapabilityFlags>( 1ULL << 63U );
    LUMEN_CHECK_HAS_ERROR( Capabilities.Validate() );
}

} // namespace Lumen
