/**
 * @file IRHIPipelineCache.hpp
 * @brief Declares synchronized pipeline lookup, insertion, and serialization.
 */
#pragma once

#include "Memory/SharedPtr.hpp"
#include <Caches/RHICacheStats.hpp>
#include <Caches/RHIPipelineCacheDesc.hpp>
#include <Memory/SharedPtr.hpp>
#include <Pipelines/IRHIPipeline.hpp>

namespace Lumen
{

/**
 * @class IRHIPipelineCache
 * @brief Backend-owned cache of immutable pipelines and opaque serialized metadata.
 */
class IRHIPipelineCache : public IRHIObject
{
public:

    [[nodiscard]] virtual const FRHIPipelineCacheDesc &GetDesc () const noexcept = 0;
    /** @return The retained pipeline on a hit, or an empty pointer on a miss. */
    [[nodiscard]] virtual TSharedPtr<IRHIPipeline> Find ( const FRHIPipelineCompatibility &InKey ) noexcept = 0;
    /** @brief Inserts or replaces a complete entry without retaining partial creations. */
    virtual TExpected<void, FRHIError> Insert ( const FRHIPipelineCompatibility &InKey,
                                                TSharedRef<IRHIPipeline> InPipeline ) noexcept = 0;
    virtual void Remove ( const FRHIPipelineCompatibility &InKey ) noexcept                    = 0;
    virtual void Clear () noexcept                                                             = 0;
    [[nodiscard]] virtual FRHICacheStats GetStats () const noexcept                            = 0;
    /** @return Versioned opaque data suitable for a later cache with the same backend version. */
    [[nodiscard]] virtual TExpected<TVector<Byte>, FRHIError> Serialize () const noexcept = 0;
};

} // namespace Lumen
