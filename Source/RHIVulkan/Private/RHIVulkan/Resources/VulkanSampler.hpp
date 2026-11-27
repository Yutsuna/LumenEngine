/**
 * @file VulkanSampler.hpp
 * @brief Declares an immutable Vulkan sampler object.
 */
#pragma once

#include <RHI/Resources/IRHISampler.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

#include <RHIVulkan/Device/VulkanDeviceContext.hpp>
#include <RHIVulkan/Types/VulkanHandle.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @class FVulkanSampler
     * @brief Native immutable VkSampler created from a validated portable descriptor.
     */
    class FVulkanSampler final : public RHI::IRHISampler
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /**
         * @brief Creates a native sampler.
         * @param InContext Shared native device context.
         * @param InDesc Validated sampler descriptor.
         * @param InId Identifier assigned by the owning device.
         * @return The sampler, or a structured error with the Resource condition.
         */
        [[nodiscard]] static TExpected<TSharedRef<FVulkanSampler>, RHI::FRHIError> Create (
            TSharedRef<FVulkanDeviceContext> InContext, const RHI::FRHISamplerDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        ~FVulkanSampler () noexcept override;

        const RHI::FRHISamplerDesc &GetDesc () const noexcept override;

        FVulkanSampler ( TSharedRef<FVulkanDeviceContext> InContext,
                         const RHI::FRHISamplerDesc &InDesc,
                         RHI::FRHIObjectId InId ) noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FVulkanDeviceContext> Context;
        RHI::FRHISamplerDesc Desc;
        FVulkanSamplerHandle Sampler;
    };

} // namespace RHIVulkan

} // namespace Lumen
