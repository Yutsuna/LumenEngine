/**
 * @file VulkanObject.inl
 * @brief Inline implementation of VulkanObject.hpp.
 */

#pragma once

#include <RHIVulkan/Types/VulkanObject.hpp>

#include <Templates/Hash.hpp>

template <class TObjectType> constexpr Lumen::UInt64 Lumen::RHIVulkan::GetVulkanBackendTypeTag () noexcept
{
    return FHash::TypeHash<TObjectType>();
}

template <typename TObjectType>
Lumen::RHI::FRHIObjectBase Lumen::RHIVulkan::MakeVulkanObjectBase ( RHI::FRHIObjectId InId ) noexcept
{
    return RHI::FRHIObjectBase( InId, GetVulkanBackendTypeTag<TObjectType>() );
}
