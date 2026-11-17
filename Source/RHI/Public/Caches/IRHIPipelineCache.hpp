/**
 * @file IRHIPipelineCache.hpp
 * @brief Declares synchronized pipeline lookup, insertion, and serialization.
 */

#pragma once

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

    /**
     * @brief Returns the immutable descriptor this cache was created with.
     * @return A reference owned by the cache, valid for its lifetime.
     */
    [[nodiscard]] virtual const FRHIPipelineCacheDesc &GetDesc () const noexcept = 0;

    /** @return The retained pipeline on a hit, or an empty pointer on a miss. */
    [[nodiscard]] virtual TSharedPtr<IRHIPipeline> Find ( const FRHIPipelineCompatibility &InKey ) noexcept = 0;

    /** @brief Inserts or replaces a complete entry without retaining partial creations. */
    virtual TExpected<void, FRHIError> Insert ( const FRHIPipelineCompatibility &InKey,
                                                TSharedRef<IRHIPipeline> InPipeline ) noexcept = 0;
    /**
     * @brief Removes the pipeline stored for a compatibility key.
     * @param InKey Canonical compatibility key to remove.
     */
    virtual void Remove ( const FRHIPipelineCompatibility &InKey ) noexcept = 0;

    /**
     * @brief Removes every cached pipeline and resets statistics.
     */
    virtual void Clear () noexcept = 0;

    /**
     * @brief Returns a coherent snapshot of cache activity.
     * @return Statistics gathered under the cache lock.
     */
    [[nodiscard]] virtual FRHICacheStats GetStats () const noexcept = 0;

    /** @return Versioned opaque data suitable for a later cache with the same backend version. */
    [[nodiscard]] virtual TExpected<TVector<Byte>, FRHIError> Serialize () const noexcept = 0;
};

} // namespace Lumen
