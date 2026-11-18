/**
 * @file RHITransientBatch.cpp
 * @brief Implements transient batch validation.
 */

#include <Memory/RHITransientBatch.hpp>

Lumen::Bool Lumen::FRHITransientDesc::Validate () const noexcept
{
    return Lifetime.IsValid() and Requirements.ToAllocationDesc().HasValue();
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHITransientBatch::Validate () const noexcept
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
