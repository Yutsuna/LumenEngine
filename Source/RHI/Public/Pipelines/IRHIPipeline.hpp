/**
 * @file IRHIPipeline.hpp
 * @brief Declares immutable graphics and compute pipelines.
 */

#pragma once

#include <Pipelines/RHIPipelineCompatibility.hpp>
#include <Types/IRHIObject.hpp>

namespace Lumen
{

/**
 * @enum ERHIPipelineType
 * @brief Pipeline work class.
 */
enum class ERHIPipelineType : UInt8
{
    Graphics,
    Compute
};

/**
 * @class IRHIPipeline
 * @brief Immutable native pipeline and compatibility metadata.
 */
class IRHIPipeline : public IRHIObject
{
public:

    /** @return Graphics or compute. */
    [[nodiscard]] virtual ERHIPipelineType GetType () const noexcept = 0;
    /** @return Canonical command/binding compatibility key. */
    [[nodiscard]] virtual const FRHIPipelineCompatibility &GetCompatibility () const noexcept = 0;
};
} // namespace Lumen
