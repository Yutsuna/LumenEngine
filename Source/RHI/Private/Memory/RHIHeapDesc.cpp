/**
 * @file RHIHeapDesc.cpp
 * @brief Implements heap descriptor validation.
 */
#include <Memory/RHIHeapDesc.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIHeapDesc::Validate () const noexcept
{
    if ( Size == 0ULL )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Heap size must be nonzero" ) );
    }
    return {};
}
