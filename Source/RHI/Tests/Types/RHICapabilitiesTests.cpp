/**
 * @file RHICapabilitiesTests.cpp
 * @brief Tests feature-mask queries and validation.
 */

#include <RHI/Types/RHICapabilities.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHICapabilities, SupportsRequiredFeatureSet )
    {
        FRHICapabilities Capabilities;
        Capabilities.Features = ERHICapabilityFlags::DynamicRendering | ERHICapabilityFlags::IndirectDraw;
        LUMEN_CHECK_TRUE( Capabilities.Supports( ERHICapabilityFlags::DynamicRendering ) );
        LUMEN_CHECK_FALSE( Capabilities.Supports( ERHICapabilityFlags::RayTracing ) );
        LUMEN_CHECK_TRUE( Capabilities.Validate().HasValue() );
        Capabilities.Features = static_cast<ERHICapabilityFlags>( 1ULL << 63U );
        LUMEN_CHECK_HAS_ERROR( Capabilities.Validate() );
    }

    LUMEN_TEST( RHICapabilities, AdvertisesUniqueKnownShaderFormats )
    {
        FRHICapabilities Capabilities;
        Capabilities.ShaderFormats = { ERHIShaderFormat::PortableBytecode, ERHIShaderFormat::Source };
        LUMEN_CHECK_TRUE( Capabilities.Supports( ERHIShaderFormat::PortableBytecode ) );
        LUMEN_CHECK_TRUE( Capabilities.Supports( ERHIShaderFormat::Source ) );
        LUMEN_CHECK_FALSE( Capabilities.Supports( ERHIShaderFormat::BackendBytecode ) );
        LUMEN_CHECK_TRUE( Capabilities.Validate().HasValue() );

        Capabilities.ShaderFormats.push_back( ERHIShaderFormat::Source );
        LUMEN_CHECK_HAS_ERROR( Capabilities.Validate() );

        Capabilities.ShaderFormats = { ERHIShaderFormat::Count };
        LUMEN_CHECK_HAS_ERROR( Capabilities.Validate() );
    }

} // namespace RHI

} // namespace Lumen
