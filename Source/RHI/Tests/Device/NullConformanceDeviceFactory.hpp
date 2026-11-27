/**
 * @file NullConformanceDeviceFactory.hpp
 * @brief Declares the Null implementation of the conformance factory.
 */

#pragma once

#include <Device/NullDeviceDesc.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>

namespace Lumen
{

namespace RHI
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
            using IRHIConformanceDeviceFactory::CreateDevice;
            TExpected<TSharedRef<IRHIDevice>, FRHIError> CreateDevice ( Bool bEnableValidation ) noexcept override;
            void SetDefaultValidation ( Bool bEnableValidation ) noexcept override;
            [[nodiscard]] Bool GetDefaultValidation () const noexcept override;
            TExpected<TSharedRef<IRHIShader>, FRHIError> CreateShader ( IRHIDevice &InDevice,
                                                                        ERHIShaderStage InStage ) const noexcept override;
            TExpected<TSharedRef<IRHISwapchain>, FRHIError>
            CreateSwapchain ( IRHIDevice &InDevice,
                              const TSharedPtr<IRHISwapchain> &InOldSwapchain = {} ) const noexcept override;
            Bool InjectDeviceLost ( IRHIDevice &InDevice ) const noexcept override;
            Bool InjectSwapchainOutOfDate ( IRHISwapchain &InSwapchain ) const noexcept override;

        private:

            FNullDeviceDesc Desc;
            Bool bDefaultValidation = false;
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
