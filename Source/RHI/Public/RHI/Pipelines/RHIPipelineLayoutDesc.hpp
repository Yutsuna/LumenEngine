/**
 * @file RHIPipelineLayoutDesc.hpp
 * @brief Defines binding and inline-constant pipeline layout.
 */

#pragma once

#include <Memory/SharedPtr.hpp>
#include <RHI/Bindings/IRHIBindingLayout.hpp>
#include <RHI/Pipelines/RHIInlineConstantRange.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHIPipelineLayoutDesc
     * @brief Ordered binding layouts and inline ranges.
     */
    struct RHI_EXPORT FRHIPipelineLayoutDesc
    {
        TVector<TSharedRef<IRHIBindingLayout>> BindingLayouts;
        TVector<FRHIInlineConstantRange> InlineConstants;

        /** @return Success for nonnull layouts and nonoverlapping valid inline ranges. */
        [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
