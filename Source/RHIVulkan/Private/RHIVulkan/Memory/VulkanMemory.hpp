/**
 * @file VulkanMemory.hpp
 * @brief Declares Vulkan memory-type selection and native memory allocation helpers.
 */
#pragma once

#include <RHI/Memory/RHIMemoryClass.hpp>
#include <RHI/Types/RHIError.hpp>

#include <Templates/Optional.hpp>

#include <RHIVulkan/Types/VulkanHandle.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @brief Returns the native property flags required by a portable memory class.
     * @param InMemoryClass Portable memory class.
     * @return Vulkan property flags that every selected memory type must satisfy.
     */
    [[nodiscard]] VkMemoryPropertyFlags GetVulkanMemoryRequiredFlags ( RHI::ERHIMemoryClass InMemoryClass ) noexcept;

    /**
     * @brief Selects a memory type compatible with a resource type-bit mask and portable class.
     * @param InPhysicalDevice Physical device owning the memory types.
     * @param InTypeBits Compatibility mask returned by a native requirements query.
     * @param InMemoryClass Portable memory class requested by the caller.
     * @return Selected memory type index, or empty when no compatible type exists.
     */
    [[nodiscard]] TOptional<UInt32> SelectVulkanMemoryTypeIndex ( VkPhysicalDevice InPhysicalDevice,
                                                                  UInt32 InTypeBits,
                                                                  RHI::ERHIMemoryClass InMemoryClass ) noexcept;

    /**
     * @brief Selects a native memory type suitable for a standalone heap of the requested size.
     * @param InPhysicalDevice Physical device owning the memory types and heaps.
     * @param InSize Required allocation size in bytes.
     * @param InMemoryClass Portable memory class requested by the caller.
     * @return Selected memory type index, or empty when no heap can satisfy the request.
     */
    [[nodiscard]] TOptional<UInt32> SelectVulkanHeapMemoryTypeIndex ( VkPhysicalDevice InPhysicalDevice,
                                                                      UInt64 InSize,
                                                                      RHI::ERHIMemoryClass InMemoryClass ) noexcept;

    /**
     * @brief Allocates native memory satisfying queried requirements and the portable memory class.
     * @param InPhysicalDevice Physical device used for type selection.
     * @param InDevice Logical device that owns the allocation.
     * @param InRequirements Native requirements reported by a resource.
     * @param InMemoryClass Portable memory class requested by the caller.
     * @return Scoped native allocation, or a structured error with the Memory condition.
     */
    [[nodiscard]] TExpected<FVulkanDeviceMemoryHandle, RHI::FRHIError>
    AllocateVulkanMemory ( VkPhysicalDevice InPhysicalDevice,
                           VkDevice InDevice,
                           const VkMemoryRequirements &InRequirements,
                           RHI::ERHIMemoryClass InMemoryClass ) noexcept;

} // namespace RHIVulkan

} // namespace Lumen
