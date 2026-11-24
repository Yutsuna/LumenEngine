/**
 * @file IRHIBindingLayout.hpp
 * @brief Declares an immutable binding layout.
 */

#pragma once

#include <RHI/Bindings/RHIBindingLayoutDesc.hpp>
#include <RHI/Types/IRHIObject.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class IRHIBindingLayout
     * @brief Immutable descriptor schema shared by sets and pipelines.
     */
    class IRHIBindingLayout : public IRHIObject
    {
    public:

        /** @return Creation description. */
        [[nodiscard]] virtual const FRHIBindingLayoutDesc &GetDesc () const noexcept = 0;
    };

} // namespace RHI

} // namespace Lumen
