/**
 * @file VulkanAdapter.cpp
 * @brief Implements Vulkan adapter enumeration and selection.
 */

#include <RHIVulkan/Module/VulkanAdapter.hpp>
#include <RHIVulkan/Module/VulkanAdapterBuilder.hpp>
#include <RHIVulkan/Module/VulkanInstance.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <Algo/Find.hpp>
#include <Algo/ForEach.hpp>

#include <utility>

/**
 * Private
 */

namespace
{

[[nodiscard]] Lumen::UInt32 GetAdapterPreferenceRank ( Lumen::RHI::ERHIAdapterType InType ) noexcept
{
    switch ( InType )
    {
    case Lumen::RHI::ERHIAdapterType::Discrete:
        return 0U;
    case Lumen::RHI::ERHIAdapterType::Integrated:
        return 1U;
    case Lumen::RHI::ERHIAdapterType::Unknown:
        return 2U;
    case Lumen::RHI::ERHIAdapterType::Software:
        return 3U;
    default:
        return 4U;
    }
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TVector<Lumen::RHIVulkan::FVulkanAdapter>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::EnumerateVulkanAdapters ( const FVulkanInstance &InInstance ) noexcept
{
    LUMEN_EXPECT( Devices, LUMEN_VK_ENUMERATE( VkPhysicalDevice, vkEnumeratePhysicalDevices, InInstance.GetHandle() ) );

    TVector<FVulkanAdapter> Result;
    for ( VkPhysicalDevice PhysicalDevice : Devices )
    {
        LUMEN_EXPECT( Adapter, BuildVulkanAdapter( InInstance, PhysicalDevice ) );
        if ( Adapter )
        {
            Result.push_back( std::move( *Adapter ) );
        }
    }

    return Result;
}

Lumen::TExpected<Lumen::RHIVulkan::FVulkanAdapter, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::SelectVulkanAdapter ( const FVulkanInstance &InInstance, TOptional<UInt64> InAdapterId ) noexcept
{
    LUMEN_EXPECT( Adapters, EnumerateVulkanAdapters( InInstance ) );

    if ( Adapters.empty() )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Device,
                                                     "No Vulkan physical device satisfies the Vulkan 1.4 baseline." ) );
    }

    if ( InAdapterId )
    {
        if ( const FVulkanAdapter *Result = Algo::FindIf( Adapters, [&] ( const FVulkanAdapter &Adapter ) noexcept -> Bool
        {
            return Adapter.Info.AdapterId == *InAdapterId;
        } ) )
        {
            return *Result;
        }
        return TUnexpected(
            RHI::MakeRHINotFound( RHI::ERHIErrorCondition::Device, "Requested Vulkan AdapterId was not found." ) );
    }

    const FVulkanAdapter *Best = &Adapters.front();
    Algo::ForEach( Adapters, [&Best] ( const FVulkanAdapter &Candidate ) noexcept
    {
        if ( GetAdapterPreferenceRank( Candidate.Info.Type ) < GetAdapterPreferenceRank( Best->Info.Type ) )
        {
            Best = &Candidate;
        }
    } );

    return *Best;
}
