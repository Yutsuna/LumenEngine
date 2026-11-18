/**
 * @file IRHIPipelineLayout.hpp
 * @brief Declares an immutable pipeline layout.
 */

#pragma once

#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <RHI/Types/IRHIObject.hpp>

namespace Lumen
{

namespace RHI
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
} // namespace RHI

} // namespace Lumen
