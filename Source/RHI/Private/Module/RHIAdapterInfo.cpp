/**
 * @file RHIAdapterInfo.cpp
 * @brief Implements adapter information validation.
 */
#include <RHI/Module/RHIAdapterInfo.hpp>

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIAdapterInfo::Validate () const noexcept
{
    if ( AdapterId == 0ULL or Name.empty() or BackendName.empty() or QueueFamilies.empty() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidDevice( "Adapter requires identity and at least one queue family" ) );
    }

    LUMEN_EXPECT( ValidateCapabilities, Capabilities.Validate() );
    LUMEN_EXPECT( ValidateLimits, Limits.Validate() );

    for ( const FRHIQueueFamilyInfo &Family : QueueFamilies )
    {
        if ( Family.Count == 0U or Family.Type >= ERHIQueueType::Count )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "Invalid adapter queue family" ) );
        }
    }
    return {};
}
