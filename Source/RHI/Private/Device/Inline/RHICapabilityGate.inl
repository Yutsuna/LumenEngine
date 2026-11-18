/**
 * @file RHICapabilityGate.inl
 * @brief Inline implementation of the private capability gate.
 */

#pragma once

#include "../RHICapabilityGate.hpp"
#include "RHI/Types/RHIError.hpp"

inline Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::CheckRHICapabilityGate ( const FRHIAdapterInfo &InAdapter,
                                                                                          const FRHIDeviceDesc &InDesc ) noexcept
{
    if ( const TExpected<void, FRHIError> Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( InDesc.AdapterId and *InDesc.AdapterId != InAdapter.AdapterId )
    {
        return TUnexpected<FRHIError>( MakeRHINotFound( ERHIErrorCondition::Device, "Requested adapter is not present" ) );
    }
    if ( not InAdapter.Capabilities.Supports( InDesc.RequiredCapabilities.Features ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( ERHIErrorCondition::Capability, "Adapter does not provide all required features" ) );
    }
    const FRHICapabilities &Required = InDesc.RequiredCapabilities;
    /** NOTE: A 0/0 requirement is unset; any nonzero request must be met by the adapter to avoid silent degradation. */
    if ( Required.ShaderModelMajor != 0U or Required.ShaderModelMinor != 0U )
    {
        if ( Required.ShaderModelMajor > InAdapter.Capabilities.ShaderModelMajor or
             ( Required.ShaderModelMajor == InAdapter.Capabilities.ShaderModelMajor and
               Required.ShaderModelMinor > InAdapter.Capabilities.ShaderModelMinor ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHIUnsupported( ERHIErrorCondition::Capability, "Adapter does not provide the required shader model" ) );
        }
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
            return TUnexpected<FRHIError>(
                MakeRHIUnsupported( ERHIErrorCondition::Queue, "Adapter does not provide the requested queue count" ) );
        }
    }
    return {};
}
