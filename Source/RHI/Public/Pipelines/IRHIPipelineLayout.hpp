/**
 * @file IRHIPipelineLayout.hpp
 * @brief Declares an immutable pipeline layout.
 */

#pragma once
#include <Pipelines/RHIPipelineLayoutDesc.hpp>
#include <Types/IRHIObject.hpp>
namespace Lumen
{

/**
 * @class IRHIPipelineLayout
 * @brief Compatibility contract shared by pipelines and binding sets.
 */
class IRHIPipelineLayout : public IRHIObject
{
public:

    /** @return Creation description. */
    [[nodiscard]] virtual const FRHIPipelineLayoutDesc &GetDesc () const noexcept = 0;
};
} // namespace Lumen
