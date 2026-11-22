/**
 * @file VulkanModule.cpp
 * @brief Implements the Vulkan RHI module.
 */

#include <RHI/Types/RHIError.hpp>
#include <RHIVulkan/Module/VulkanModule.hpp>

#include <Templates/Unexpected.hpp>

/**
 * Static
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIModule>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanModule::Create () noexcept
{
    return MakeShared<FVulkanModule>();
}

/**
 * Public
 */

Lumen::RHI::FRHIApiVersion Lumen::RHIVulkan::FVulkanModule::GetApiVersion () const noexcept
{
    /** TODO: Remains 1.0.0 until RHI updates its contract to 1.1.0. */
    return { .Major = 1U, .Minor = 0U, .Patch = 0U };
}

Lumen::TExpected<Lumen::TVector<Lumen::RHI::FRHIAdapterInfo>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanModule::EnumerateAdapters () const noexcept
{
    return TUnexpected(
        RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Device, "Vulkan adapter enumeration is not implemented yet." ) );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDevice>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanModule::CreateDevice ( const RHI::FRHIDeviceDesc &InDesc ) noexcept
{
    using FReturnType = TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError>;

    return InDesc.Validate().AndThen( [] () noexcept -> FReturnType
    {
        return MakeUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Device, "Vulkan device creation is not implemented yet." ) );
    } );
}
