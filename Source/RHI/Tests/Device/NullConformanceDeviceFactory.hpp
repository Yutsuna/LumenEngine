/**
 * @file NullConformanceDeviceFactory.hpp
 * @brief Declares the Null implementation of the conformance factory.
 */

#pragma once

#include <Device/NullDeviceDesc.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>

namespace Lumen
{

namespace Tests
{

    /**
     * @class FNullConformanceDeviceFactory
     * @brief Creates configurable Null modules and devices.
     */
    class FNullConformanceDeviceFactory final : public IRHIConformanceDeviceFactory
    {
    public:

        /** @brief Uses default Null capabilities, or the supplied adapter/fault configuration. */
        explicit FNullConformanceDeviceFactory ( FNullDeviceDesc InDesc = {} ) noexcept;
        TSharedRef<IRHIModule> GetModule () const noexcept override;
        TExpected<TSharedRef<IRHIDevice>, FRHIError> CreateDevice ( Bool bEnableValidation = false ) noexcept override;

    private:

        FNullDeviceDesc Desc;
    };

} // namespace Tests

} // namespace Lumen
