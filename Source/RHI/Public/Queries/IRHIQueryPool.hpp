/**
 * @file IRHIQueryPool.hpp
 * @brief Declares query reset and completed-result readback.
 */
#pragma once

#include <Memory/Span.hpp>
#include <Queries/RHIQueryPoolDesc.hpp>
#include <Types/IRHIObject.hpp>
#include <Types/RHITimeline.hpp>

namespace Lumen
{

/**
 * @class IRHIQueryPool
 * @brief Backend storage for a homogeneous array of queries.
 */
class IRHIQueryPool : public IRHIObject
{
public:

    /**
     * @brief Returns the immutable query pool descriptor.
     * @return A reference owned by the pool.
     */
    [[nodiscard]] virtual const FRHIQueryPoolDesc &GetDesc () const noexcept = 0;
    /** @brief Makes every result unavailable; the caller must externally order this with GPU use. */
    virtual void Reset () noexcept = 0;
    /** @brief Copies completed results, failing if a requested query is unavailable at InCompleted. */
    [[nodiscard]] virtual TExpected<void, FRHIError> ReadResults ( UInt32 InFirstQuery,
                                                                   UInt32 InQueryCount,
                                                                   TSpan<UInt64> InDestination,
                                                                   FRHITimelineValue InCompleted ) const noexcept = 0;
};

} // namespace Lumen
