/**
 * @file RHIVulkanModule.hpp
 * @brief Declares the Vulkan RHI module entry point.
 */

#pragma once

#include <RHIVulkanExport.hpp>

#include <RHI/Module/IRHIModule.hpp>
#include <RHI/Types/RHIError.hpp>

#include <Memory/SharedPtr.hpp>
#include <Templates/Expected.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @brief Creates and initializes the Vulkan RHI module instance.
     * @return A `TExpected` containing the initialized module instance or an error.
     */
    [[nodiscard]] RHIVULKAN_EXPORT TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> CreateRHIVulkanModule () noexcept;

} // namespace RHIVulkan

} // namespace Lumen
