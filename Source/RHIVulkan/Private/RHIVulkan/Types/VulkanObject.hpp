/**
 * @file VulkanObject.hpp
 * @brief Declares backend object identity and IRHIObject forwarding helpers for RHIVulkan.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <RHI/Types/RHIIdentifiers.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @return Stable backend type tag matching TryRHIBackendCast<TObjectType>.
     * @tparam TObjectType The backend object type to get the tag for.
     */
    template <class TObjectType>
    [[nodiscard]]
    constexpr UInt64 GetVulkanBackendTypeTag () noexcept;

    /**
     * @brief Make a FRHIObjectBase bound to a concrete RHIVulkan object type tag.
     * @tparam TObjectType The backend object type to create the FRHIObjectBase for.
     */
    template <class TObjectType> [[nodiscard]] RHI::FRHIObjectBase MakeVulkanObjectBase ( RHI::FRHIObjectId InId ) noexcept;

} // namespace RHIVulkan

} // namespace Lumen

#include "Inline/VulkanObject.inl"
