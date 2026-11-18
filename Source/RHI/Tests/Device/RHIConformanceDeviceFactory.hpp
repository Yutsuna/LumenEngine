/**
 * @file RHIConformanceDeviceFactory.hpp
 * @brief Declares the injectable factory shared by backend conformance suites.
 */

#pragma once

#include <Memory/SharedPtr.hpp>
#include <RHI/Module/IRHIModule.hpp>
#include <RHI/Types/RHIShaderStage.hpp>

namespace Lumen
{

namespace RHI
{

    class IRHIShader;
    class IRHISwapchain;

    namespace Tests
    {

        /**
         * @class IRHIConformanceDeviceFactory
         * @brief Supplies one backend to portable scenarios.
         */
        class IRHIConformanceDeviceFactory
        {
        public:

            virtual ~IRHIConformanceDeviceFactory () noexcept = default;

            /** @return The backend module used for adapter discovery. */
            [[nodiscard]] virtual TSharedRef<IRHIModule> GetModule () const noexcept = 0;
            /** @brief Creates a device, optionally enclosed by validation. */
            [[nodiscard]] virtual TExpected<TSharedRef<IRHIDevice>, FRHIError>
            CreateDevice ( Bool bEnableValidation = false ) noexcept = 0;
            /** @brief Creates a valid backend shader for portable pipeline scenarios. */
            [[nodiscard]] virtual TExpected<TSharedRef<IRHIShader>, FRHIError>
            CreateShader ( IRHIDevice &InDevice, ERHIShaderStage InStage ) const noexcept = 0;
            /** @brief Creates a swapchain on a backend-supplied surface, optionally replacing an old one. */
            [[nodiscard]] virtual TExpected<TSharedRef<IRHISwapchain>, FRHIError>
            CreateSwapchain ( IRHIDevice &InDevice, const TSharedPtr<IRHISwapchain> &InOldSwapchain = {} ) const noexcept = 0;
            /** @return True when the backend injected device loss for error-path scenarios. */
            [[nodiscard]] virtual Bool InjectDeviceLost ( IRHIDevice &InDevice ) const noexcept = 0;
            /** @return True when the backend marked the swapchain out of date for error-path scenarios. */
            [[nodiscard]] virtual Bool InjectSwapchainOutOfDate ( IRHISwapchain &InSwapchain ) const noexcept = 0;
        };

        /** @return The factory linked into the current backend test binary. */
        [[nodiscard]] IRHIConformanceDeviceFactory &GetRHIConformanceDeviceFactory () noexcept;

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
