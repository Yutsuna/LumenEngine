/**
 * @file VulkanCore.hpp
 * @brief Core utilities and macros for the Vulkan RHI module.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Logging/Logger.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

extern const LUMEN_ENGINE_API FLogCategory LogVulkanRHI;

namespace RHI
{

/**
 * @brief Macro to check Vulkan results and fatally crash with context if an API call fails.
 * @details Prepares the engine for robust failure tracking.
 */
#define VULKAN_CHECK( Expr )                                                                                                                                             \
    do                                                                                                                                                                   \
    {                                                                                                                                                                    \
        const VkResult Macro_VulkanResult = ( Expr );                                                                                                                    \
        if ( Macro_VulkanResult != VK_SUCCESS )                                                                                                                          \
        {                                                                                                                                                                \
            LUMEN_LOG_FATAL( ::LumenEngine::LogVulkanRHI, "Vulkan API call failed with code: {} at {}:{}", static_cast<Int32>( Macro_VulkanResult ), __FILE__,           \
                             __LINE__ );                                                                                                                                 \
        }                                                                                                                                                                \
    } while ( 0 )

    /** Number of frames strictly kept in flight for triple buffering / deferred deletion */
    constexpr UInt32 MAX_FRAMES_IN_FLIGHT = 3;

} // namespace RHI

} // namespace LumenEngine
