/**
 * @file RHIPipelineCacheDesc.hpp
 * @brief Defines pipeline cache limits and opaque initial data.
 */
#pragma once

#include <Memory/Span.hpp>
#include <RHIExport.hpp>
#include <Types/RHIError.hpp>

namespace Lumen
{

/**
 * @struct FRHIPipelineCacheDesc
 * @brief Bounded backend-versioned pipeline cache configuration.
 */
struct RHI_EXPORT FRHIPipelineCacheDesc
{
    UInt32 MaxEntries     = 128U;
    UInt64 MaxBytes       = 16ULL * 1024ULL * 1024ULL;
    UInt64 BackendVersion = 1ULL;
    TSpan<const Byte> InitialData;
    Bool bAllowEviction = true;

    /** @return Success when limits are nonzero and initial data fits the declared budget. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};

} // namespace Lumen
