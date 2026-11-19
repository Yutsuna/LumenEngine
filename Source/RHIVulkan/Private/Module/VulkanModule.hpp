/**
 * @file VulkanModule.hpp
 * @brief Declares the Vulkan RHI module implementation.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <RHI/Module/IRHIModule.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @class FVulkanModule
     * @brief Vulkan Backend module responsible for discovery and device creation.
     */
    class FVulkanModule final : public RHI::IRHIModule, public FNonCopyable, public FNonMovable
    {
    public:

        FVulkanModule () noexcept           = default;
        ~FVulkanModule () noexcept override = default;

    public:

        /** @brief Creates a Vulkan RHI module instance */
        [[nodiscard]] static TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Create () noexcept;

        /** @return API version implemented by this module. */
        [[nodiscard]] RHI::FRHIApiVersion GetApiVersion () const noexcept override;

        /** @return All currently visible adapters, or a discovery failure. */
        [[nodiscard]] TExpected<TVector<RHI::FRHIAdapterInfo>, RHI::FRHIError> EnumerateAdapters () const noexcept override;

        /** @brief Creates a device after validating the requested adapter and capabilities. */
        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError>
        CreateDevice ( const RHI::FRHIDeviceDesc &InDesc ) noexcept override;
    };

} // namespace RHIVulkan

} // namespace Lumen
