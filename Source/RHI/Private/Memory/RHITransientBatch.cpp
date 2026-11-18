/**
 * @file RHITransientBatch.cpp
 * @brief Implements transient batch validation.
 */

#include <RHI/Memory/RHITransientBatch.hpp>

Lumen::Bool Lumen::RHI::FRHITransientDesc::Validate () const noexcept
{
    return Lifetime.IsValid() and Requirements.ToAllocationDesc().HasValue();
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHITransientBatch::Validate () const noexcept
{
    for ( const FRHITransientDesc &Request : Requests )
    {
        if ( not Request.Validate() )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Invalid transient request" ) );
        }
    }
    return {};
}
