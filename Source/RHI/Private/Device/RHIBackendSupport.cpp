/**
 * @file RHIBackendSupport.cpp
 * @brief Implements the RHI backend support functions.
 */

#include <RHI/Device/RHIBackendSupport.hpp>
#include <RHI/Types/RHIShaderFormat.hpp>

#include <Algo/Any.hpp>
#include <Algo/ForEach.hpp>

/**
 * Private
 */

namespace
{

inline Lumen::RHI::FRHIExpected ValidateAdapterId ( const Lumen::RHI::FRHIAdapterInfo &InAdapter,
                                                    const Lumen::RHI::FRHIDeviceDesc &InDesc ) noexcept
{
    if ( InDesc.AdapterId and *InDesc.AdapterId != InAdapter.AdapterId )
    {
        return Lumen::MakeUnexpected(
            MakeRHINotFound( Lumen::RHI::ERHIErrorCondition::Device, "Requested adapter is not present" ) );
    }
    return {};
}

inline Lumen::RHI::FRHIExpected ValidateFeatures ( const Lumen::RHI::FRHIAdapterInfo &InAdapter,
                                                   const Lumen::RHI::FRHIDeviceDesc &InDesc ) noexcept
{
    if ( not InAdapter.Capabilities.Supports( InDesc.RequiredCapabilities.Features ) )
    {
        return Lumen::MakeUnexpected(
            MakeRHIUnsupported( Lumen::RHI::ERHIErrorCondition::Capability, "Adapter does not provide all required features" ) );
    }
    return {};
}

inline Lumen::RHI::FRHIExpected ValidateShaderFormats ( const Lumen::RHI::FRHIAdapterInfo &InAdapter,
                                                        const Lumen::RHI::FRHICapabilities &Required ) noexcept
{
    if ( Lumen::Algo::AnyOf( Required.ShaderFormats, [&InAdapter] ( const Lumen::RHI::ERHIShaderFormat Format )
    {
        return not InAdapter.Capabilities.Supports( Format );
    } ) )
    {
        return Lumen::MakeUnexpected(
            MakeRHIUnsupported( Lumen::RHI::ERHIErrorCondition::Shader, "Adapter does not provide a required shader format" ) );
    }
    return {};
}

/** NOTE: A 0/0 requirement is unset; any nonzero request must be met by the adapter to avoid silent degradation. */
inline Lumen::RHI::FRHIExpected ValidateShaderModel ( const Lumen::RHI::FRHIAdapterInfo &InAdapter,
                                                      const Lumen::RHI::FRHICapabilities &Required ) noexcept
{
    if ( Required.ShaderModelMajor == 0U and Required.ShaderModelMinor == 0U )
    {
        return {};
    }

    const Lumen::Bool bMajorLower     = Required.ShaderModelMajor < InAdapter.Capabilities.ShaderModelMajor;
    const Lumen::Bool bMajorEqual     = Required.ShaderModelMajor == InAdapter.Capabilities.ShaderModelMajor;
    const Lumen::Bool bMinorNotHigher = Required.ShaderModelMinor <= InAdapter.Capabilities.ShaderModelMinor;

    if ( bMajorLower or ( bMajorEqual and bMinorNotHigher ) )
    {
        return {};
    }

    return Lumen::MakeUnexpected(
        MakeRHIUnsupported( Lumen::RHI::ERHIErrorCondition::Capability, "Adapter does not provide the required shader model" ) );
}

Lumen::RHI::FRHIExpected ValidateQueues ( const Lumen::RHI::FRHIAdapterInfo &InAdapter,
                                          const Lumen::RHI::FRHIDeviceDesc &InDesc ) noexcept
{
    for ( const Lumen::RHI::FRHIQueueRequest &Request : InDesc.Queues )
    {
        Lumen::UInt32 Available = 0U;
        Lumen::Algo::ForEach( InAdapter.QueueFamilies, [&Request, &Available] ( const Lumen::RHI::FRHIQueueFamilyInfo &Family )
        {
            if ( Family.Type == Request.Type )
            {
                Available += Family.Count;
            }
        } );
        if ( Request.Count > Available )
        {
            return Lumen::MakeUnexpected( MakeRHIUnsupported( Lumen::RHI::ERHIErrorCondition::Queue,
                                                              "Adapter does not provide the requested queue count" ) );
        }
    }
    return {};
}

} // namespace

/**
 * Public
 */

Lumen::RHI::FRHIExpected Lumen::RHI::CheckRHICapabilityGate ( const Lumen::RHI::FRHIAdapterInfo &InAdapter,
                                                              const Lumen::RHI::FRHIDeviceDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( _, InDesc.Validate() );
    LUMEN_EXPECT( CapabilityValidation, InDesc.RequiredCapabilities.Validate() );

    LUMEN_EXPECT( AdapterIdCheck, ValidateAdapterId( InAdapter, InDesc ) );
    LUMEN_EXPECT( FeaturesCheck, ValidateFeatures( InAdapter, InDesc ) );
    LUMEN_EXPECT( ShaderFormatsCheck, ValidateShaderFormats( InAdapter, InDesc.RequiredCapabilities ) );
    LUMEN_EXPECT( ShaderModelCheck, ValidateShaderModel( InAdapter, InDesc.RequiredCapabilities ) );
    LUMEN_EXPECT( QueuesCheck, ValidateQueues( InAdapter, InDesc ) );

    return {};
}
