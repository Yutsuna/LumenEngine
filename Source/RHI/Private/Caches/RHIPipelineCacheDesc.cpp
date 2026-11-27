/**
 * @file RHIPipelineCacheDesc.cpp
 * @brief Implements pipeline cache descriptor validation.
 */

#include <RHI/Caches/RHIPipelineCacheDesc.hpp>

/**
 * Public
 */

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIPipelineCacheDesc::Validate () const noexcept
{
    if ( MaxEntries == 0U or MaxBytes == 0ULL or BackendVersion == 0ULL or static_cast<UInt64>( InitialData.size() ) > MaxBytes )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidCache( "Invalid pipeline cache capacity, version, or initial data" ) );
    }
    return {};
}
