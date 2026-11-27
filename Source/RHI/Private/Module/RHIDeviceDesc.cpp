/**
 * @file RHIDeviceDesc.cpp
 * @brief Implements device descriptor validation.
 */
#include <RHI/Module/RHIDeviceDesc.hpp>

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIDeviceDesc::Validate () const noexcept
{
    if ( Queues.empty() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "At least one queue is required" ) );
    }
    for ( USize First = 0U; First < Queues.size(); ++First )
    {
        if ( Queues[First].Count == 0U or Queues[First].Type >= ERHIQueueType::Count )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "Queue request must be valid and nonzero" ) );
        }
        for ( USize Second = First + 1U; Second < Queues.size(); ++Second )
        {
            if ( Queues[First].Type == Queues[Second].Type )
            {
                return TUnexpected<FRHIError>( MakeRHIInvalidQueue( "Duplicate queue type request" ) );
            }
        }
    }
    return {};
}
