/**
 * @file VulkanModule.cpp
 * @brief Implements the Vulkan RHI module.
 */

#include <RHI/Module/RHIAdapterInfo.hpp>
#include <RHIVulkan/Device/VulkanDevice.hpp>
#include <RHIVulkan/Module/VulkanAdapter.hpp>
#include <RHIVulkan/Module/VulkanInstance.hpp>

#include <RHI/Types/RHIError.hpp>
#include <RHIVulkan/Module/VulkanModule.hpp>

#include <Algo/ForEach.hpp>
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
    return { .Major = 1U, .Minor = 1U, .Patch = 0U };
}

Lumen::TExpected<Lumen::TVector<Lumen::RHI::FRHIAdapterInfo>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanModule::EnumerateAdapters () const noexcept
{
    LUMEN_EXPECT( Instance, FVulkanInstance::Create( false ) );
    LUMEN_EXPECT( Adapters, EnumerateVulkanAdapters( *Instance ) );

    TVector<RHI::FRHIAdapterInfo> OutInfos;
    OutInfos.reserve( Adapters.size() );
    Algo::ForEach( Adapters, [&OutInfos] ( const FVulkanAdapter &Adapter ) noexcept -> void
    {
        OutInfos.push_back( Adapter.Info );
    } );

    return OutInfos;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDevice>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanModule::CreateDevice ( const RHI::FRHIDeviceDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Instance, FVulkanInstance::Create( InDesc.bEnableDebug ) );
    LUMEN_EXPECT( Adapters, SelectVulkanAdapter( *Instance, InDesc.AdapterId ) );

    return FVulkanDevice::Create( Instance, Adapters, InDesc );
}
