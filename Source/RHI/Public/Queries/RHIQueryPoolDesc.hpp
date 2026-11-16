/**
 * @file RHIQueryPoolDesc.hpp
 * @brief Defines query pool capacity and queue compatibility.
 */
#pragma once

#include <Queries/RHIQueryType.hpp>
#include <RHIExport.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIQueueType.hpp>

namespace Lumen
{

/**
 * @struct FRHIQueryPoolDesc
 * @brief Immutable query type, count, and recording queue.
 */
struct RHI_EXPORT FRHIQueryPoolDesc
{
    ERHIQueryType Type      = ERHIQueryType::Occlusion;
    UInt32 Count            = 1U;
    ERHIQueueType QueueType = ERHIQueueType::Graphics;

    /** @return Success when the type, count, and queue combination is valid. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    /** @return Byte size of one result in this pool. */
    [[nodiscard]] UInt32 GetResultSize () const noexcept;
};

} // namespace Lumen
