/**
 * @file RHIAdapterInfo.cpp
 * @brief Implements adapter information validation.
 */
#include <RHI/Module/RHIAdapterInfo.hpp>

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIAdapterInfo::Validate () const noexcept
{
    if ( AdapterId == 0ULL or Name.empty() or BackendName.empty() or QueueFamilies.empty() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidDevice( "Adapter requires identity and at least one queue family" ) );
    }
    if ( not Capabilities.Validate() )
    {
        return TUnexpected<FRHIError>( Capabilities.Validate().Error() );
    }
    if ( not Limits.Validate() )
    {
        return TUnexpected<FRHIError>( Limits.Validate().Error() );
    }
    for ( const FRHIQueueFamilyInfo &Family : QueueFamilies )
    {
        if ( Family.Count == 0U or Family.Type >= ERHIQueueType::Count )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "Invalid adapter queue family" ) );
        }
    }
    return {};
}
