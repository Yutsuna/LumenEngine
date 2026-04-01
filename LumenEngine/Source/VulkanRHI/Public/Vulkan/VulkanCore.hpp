/**
 * @file VulkanCore.hpp
 * @brief Utilities and core definitions for Vulkan RHI
 */

#pragma once

#include "Logging/Logger.hpp"
#include "Logging/LoggingCategory.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    extern LUMEN_ENGINE_API const FLogCategory LogVulkanRHI;

    static constexpr UInt32 MaxFramesInFlight = 2;
    static constexpr UInt32 NullBindlessID    = 0xFFFFFFFF;

} // namespace VulkanRHI

} // namespace LumenEngine

/**
 * @def LUMEN_VK_CHECK(result)
 * @brief Checks a Vulkan result and logs a fatal error if it fails.
 */
#define LUMEN_VK_CHECK( InCall )                                                                                                                                         \
                                                                                                                                                                         \
    do                                                                                                                                                                   \
    {                                                                                                                                                                    \
        const VkResult Result = ( InCall );                                                                                                                              \
        if ( Result != VK_SUCCESS )                                                                                                                                      \
        {                                                                                                                                                                \
            LUMEN_LOG_FATAL( ::LumenEngine::VulkanRHI::LogVulkanRHI, "Vulkan Error {} at {}:{}", static_cast<::LumenEngine::Int32>( Result ), __FILE__, __LINE__ );      \
        }                                                                                                                                                                \
    } while ( 0 )
