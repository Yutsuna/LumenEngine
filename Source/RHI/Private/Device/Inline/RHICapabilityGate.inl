/**
 * @file RHICapabilityGate.inl
 * @brief Inline implementation of the private capability gate.
 */

#pragma once

#include "../RHICapabilityGate.hpp"

inline Lumen::TExpected<void, Lumen::FRHIError> Lumen::CheckRHICapabilityGate ( const FRHIAdapterInfo &InAdapter,
                                                                                const FRHIDeviceDesc &InDesc ) noexcept
{
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( InDesc.AdapterId and *InDesc.AdapterId != InAdapter.AdapterId )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( NotFound, ERHIErrorCondition::Device, "Requested adapter is not present" ) );
    }
    if ( not InAdapter.Capabilities.Supports( InDesc.RequiredCapabilities.Features ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( Unsupported, ERHIErrorCondition::Capability, "Adapter does not provide all required features" ) );
    }
    for ( const FRHIQueueRequest &Request : InDesc.Queues )
    {
        UInt32 Available = 0U;
        for ( const FRHIQueueFamilyInfo &Family : InAdapter.QueueFamilies )
        {
            if ( Family.Type == Request.Type )
            {
                Available += Family.Count;
            }
        }
        if ( Request.Count > Available )
        {
            return TUnexpected<FRHIError>( MakeRHIUnsupported( Unsupported, ERHIErrorCondition::Queue,
                                                               "Adapter does not provide the requested queue count" ) );
        }
    }
    return {};
}
