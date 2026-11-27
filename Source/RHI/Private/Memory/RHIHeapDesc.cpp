/**
 * @file RHIHeapDesc.cpp
 * @brief Implements heap descriptor validation.
 */

#include <RHI/Memory/RHIHeapDesc.hpp>

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIHeapDesc::Validate () const noexcept
{
    if ( Size == 0ULL )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Heap size must be nonzero" ) );
    }
    return {};
}
