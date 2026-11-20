/**
 * @file VulkanObject.hpp
 * @brief Declares backend object identity and IRHIObject forwarding helpers for RHIVulkan.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <RHI/Types/RHIIdentifiers.hpp>
#include <RHI/Types/RHIObjectBase.hpp>

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
    constexpr UInt64 GetVulkanBackendTypeTag  () noexcept;

    /**
     * @brief Make a FRHIObjectBase bound to a concrete RHIVulkan object type tag.
     * @tparam TObjectType The backend object type to create the FRHIObjectBase for.
     */
    template <class TObjectType> [[nodiscard]] RHI::FRHIObjectBase MakeVulkanObjectBase ( RHI::FRHIObjectId InId ) noexcept;

/**
 * @macro LUMEN_VK_OBJECT_FORWARDERS
 * @brief Forwards IRHIObject methods and exposes GetRHIObjectBase() for FRHIDeviceBase::TrackObject.
 */
#define LUMEN_VK_OBJECT_FORWARDERS( ObjectBaseMember )                                                                           \
    [[nodiscard]] Lumen::RHI::FRHIObjectId GetId() const noexcept override                                                       \
    {                                                                                                                            \
        return ( ObjectBaseMember ).GetId();                                                                                     \
    }                                                                                                                            \
    [[nodiscard]] Lumen::FString GetDebugName() const noexcept override                                                          \
    {                                                                                                                            \
        return ( ObjectBaseMember ).GetDebugName();                                                                              \
    }                                                                                                                            \
    void SetDebugName( Lumen::FStringView InName ) noexcept override                                                             \
    {                                                                                                                            \
        ( ObjectBaseMember ).SetDebugName( InName );                                                                             \
    }                                                                                                                            \
    [[nodiscard]] Lumen::UInt64 GetBackendTypeTag() const noexcept override                                                      \
    {                                                                                                                            \
        return ( ObjectBaseMember ).GetBackendTypeTag();                                                                         \
    }                                                                                                                            \
    [[nodiscard]] Lumen::RHI::FRHIObjectBase &GetRHIObjectBase() noexcept                                                        \
    {                                                                                                                            \
        return ( ObjectBaseMember );                                                                                             \
    }                                                                                                                            \
    [[nodiscard]] const Lumen::RHI::FRHIObjectBase &GetRHIObjectBase() const noexcept                                            \
    {                                                                                                                            \
        return ( ObjectBaseMember );                                                                                             \
    }

} // namespace RHIVulkan

} // namespace Lumen

#include "Inline/VulkanObject.inl"
