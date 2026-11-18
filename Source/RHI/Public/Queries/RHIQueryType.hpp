/**
 * @file RHIQueryType.hpp
 * @brief Defines query classes and their portable result layout.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

/**
 * @enum ERHIQueryType
 * @brief Measurements that can be recorded by command lists.
 */
enum class ERHIQueryType : UInt8
{
    Occlusion,
    Timestamp,
    PipelineStatistics,
    Count
};

/** @return Number of UInt64 values produced for one query, or empty for an invalid type. */
[[nodiscard]] RHI_EXPORT TOptional<UInt32> GetRHIQueryValueCount ( ERHIQueryType InType ) noexcept;
/** @return Byte size of one query result, or empty for an invalid type. */
[[nodiscard]] RHI_EXPORT TOptional<UInt32> GetRHIQueryResultSize ( ERHIQueryType InType ) noexcept;

} // namespace Lumen
