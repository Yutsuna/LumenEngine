/**
 * @file OpenGLConformanceDeviceFactory.hpp
 * @brief Declares the OpenGL conformance device factory.
 */

#pragma once

#include <Device/RHIConformanceDeviceFactory.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    namespace Tests
    {

        /**
         * @class FOpenGLConformanceDeviceFactory
         * @brief Supplies OpenGL modules and devices to the portable conformance suites.
         */
        class FOpenGLConformanceDeviceFactory final : public RHI::Tests::IRHIConformanceDeviceFactory
        {
        public:

            FOpenGLConformanceDeviceFactory () noexcept = default;

            [[nodiscard]] TSharedRef<RHI::IRHIModule> GetModule () const noexcept override;
            using RHI::Tests::IRHIConformanceDeviceFactory::CreateDevice;
            [[nodiscard]] TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError>
            CreateDevice ( Bool bEnableValidation ) noexcept override;
            void SetDefaultValidation ( Bool bEnableValidation ) noexcept override;
            [[nodiscard]] Bool GetDefaultValidation () const noexcept override;
            [[nodiscard]] TExpected<TSharedRef<RHI::IRHIShader>, RHI::FRHIError>
            CreateShader ( RHI::IRHIDevice &InDevice, RHI::ERHIShaderStage InStage ) const noexcept override;
            [[nodiscard]] TExpected<TSharedRef<RHI::IRHISwapchain>, RHI::FRHIError>
            CreateSwapchain ( RHI::IRHIDevice &InDevice,
                              const TSharedPtr<RHI::IRHISwapchain> &InOldSwapchain = {} ) const noexcept override;
            [[nodiscard]] Bool InjectDeviceLost ( RHI::IRHIDevice &InDevice ) const noexcept override;
            [[nodiscard]] Bool InjectSwapchainOutOfDate ( RHI::IRHISwapchain &InSwapchain ) const noexcept override;

        private:

            Bool bDefaultValidation = false;
        };

    } // namespace Tests

} // namespace RHIOpenGL

} // namespace Lumen
