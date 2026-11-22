/**
 * @file VulkanResult.hpp
 * @brief Declares exhaustive mapping from VkResult to structured RHI errors.
 */

#pragma once

#include <RHI/Types/RHIError.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

/**
 * @macro LUMEN_VK_ERROR_RESULT_LIST
 * @brief List of Vulkan result codes and their corresponding RHI error codes.
 */
#define LUMEN_VK_ERROR_RESULT_LIST( X )                                                                                          \
    X( VK_TIMEOUT, Timeout )                                                                                                     \
    X( VK_INCOMPLETE, CorruptedData )                                                                                            \
    X( VK_ERROR_OUT_OF_HOST_MEMORY, OutOfMemory )                                                                                \
    X( VK_ERROR_OUT_OF_DEVICE_MEMORY, OutOfMemory )                                                                              \
    X( VK_ERROR_OUT_OF_POOL_MEMORY, OutOfMemory )                                                                                \
    X( VK_ERROR_FRAGMENTATION, OutOfMemory )                                                                                     \
    X( VK_ERROR_TOO_MANY_OBJECTS, OutOfMemory )                                                                                  \
    X( VK_ERROR_MEMORY_MAP_FAILED, BackendFailure )                                                                              \
    X( VK_ERROR_INITIALIZATION_FAILED, BackendFailure )                                                                          \
    X( VK_ERROR_UNKNOWN, BackendFailure )                                                                                        \
    X( VK_ERROR_INCOMPATIBLE_DRIVER, Unsupported )                                                                               \
    X( VK_ERROR_LAYER_NOT_PRESENT, Unsupported )                                                                                 \
    X( VK_ERROR_EXTENSION_NOT_PRESENT, Unsupported )                                                                             \
    X( VK_ERROR_FEATURE_NOT_PRESENT, Unsupported )                                                                               \
    X( VK_ERROR_NOT_PERMITTED, Unsupported )                                                                                     \
    X( VK_ERROR_DEVICE_LOST, DeviceLost )                                                                                        \
    X( VK_ERROR_VALIDATION_FAILED_EXT, ValidationFailed )                                                                        \
    X( VK_ERROR_INVALID_SHADER_NV, InvalidArgument )                                                                             \
    X( VK_ERROR_INVALID_EXTERNAL_HANDLE, InvalidArgument )                                                                       \
    X( VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT, InvalidState )                                                                    \
    X( VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, InvalidState )                                                                         \
    X( VK_ERROR_SURFACE_LOST_KHR, OutOfDate )                                                                                    \
    X( VK_ERROR_OUT_OF_DATE_KHR, OutOfDate )

    /**
     * @brief Translates a Vulkan result code to an RHI error code.
     * @param InResult The Vulkan result code to translate.
     * @param bNotReadyAsTimeout Whether the result code indicates not ready as a timeout.
     * @return The translated RHI error code.
     */
    [[nodiscard]] constexpr RHI::ERHIErrorCode TranslateVulkanResultCode ( VkResult InResult, Bool bNotReadyAsTimeout ) noexcept;

    /**
     * @brief Returns whether the Vulkan result code indicates a success.
     * @param InResult The Vulkan result code to check.
     * @return Whether the result code indicates a success.
     */
    [[nodiscard]] constexpr Bool IsVulkanResultSuccess ( VkResult InResult ) noexcept;

    /**
     * @brief Translates a non-success VkResult into an FRHIError with NativeCode preserved.
     * @param InResult Native Vulkan result code.
     * @param InCondition RHI subsystem condition supplied by the failing operation.
     * @param InOperation Context message describing the failed operation.
     * @param bNotReadyAsTimeout When true, maps VK_NOT_READY to Timeout rather than InvalidState.
     */
    [[nodiscard]] RHI::FRHIError MapVulkanResult ( VkResult InResult,
                                                   RHI::ERHIErrorCondition InCondition,
                                                   FStringView InOperation,
                                                   Bool bNotReadyAsTimeout = false ) noexcept;

/**
 * @macro LUMEN_VK_RESULT_STR
 * @brief Converts a Vulkan result code to a string.
 */
#define LUMEN_VK_RESULT_STR( Result ) #Result

/**
 * @macro LUMEN_VK_TRY_IMPL
 * @brief Evaluate the expression and propagate the error if the result is not a success.
 */
#define LUMEN_VK_TRY_IMPL( Expr, Condition, NotReadyAsTimeout )                                                                  \
    do                                                                                                                           \
    {                                                                                                                            \
        const VkResult LumenVkResult_ = ( Expr );                                                                                \
        if ( not ::Lumen::RHIVulkan::IsVulkanResultSuccess( LumenVkResult_ ) )                                                   \
        {                                                                                                                        \
            return ::Lumen::TUnexpected(                                                                                         \
                ::Lumen::RHIVulkan::MapVulkanResult( LumenVkResult_, ( Condition ), #Expr, ( NotReadyAsTimeout ) ) );            \
        }                                                                                                                        \
    } while ( false )

/**
 * @macro LUMEN_VK_TRY
 * @brief Propagate error ; context = Vulkan call text, automatically.
 */
#define LUMEN_VK_TRY( Expr, Condition ) LUMEN_VK_TRY_IMPL( Expr, Condition, false )

/**
 * @macro LUMEN_VK_TRY_WAIT
 * @brief Propagate error ; context = Vulkan call text, automatically.
 * @note VK_NOT_READY / VK_TIMEOUT → Timeout (fences, waits).
 */
#define LUMEN_VK_TRY_WAIT( Expr, Condition ) LUMEN_VK_TRY_IMPL( Expr, Condition, true )

/**
 * @macro LUMEN_VK_TRY_PRESENT
 * @brief Propagate error ; context = Vulkan call text, automatically.
 * @note VK_SUBOPTIMAL_KHR → Suboptimal (swapchain, presentation).
 */
#define LUMEN_VK_TRY_PRESENT( Expr, OutSuboptimal )                                                                              \
    do                                                                                                                           \
    {                                                                                                                            \
        const VkResult LumenVkResult_ = ( Expr );                                                                                \
        ( OutSuboptimal )             = ( LumenVkResult_ == VK_SUBOPTIMAL_KHR );                                                 \
        if ( not ::Lumen::RHIVulkan::IsVulkanSwapchainResultSuccess( LumenVkResult_ ) )                                          \
        {                                                                                                                        \
            return ::Lumen::TUnexpected( ::Lumen::RHIVulkan::MapVulkanResult(                                                    \
                LumenVkResult_, ::Lumen::RHI::ERHIErrorCondition::Presentation, #Expr, false ) );                                \
        }                                                                                                                        \
    } while ( false )

} // namespace RHIVulkan

} // namespace Lumen

#include "Inline/VulkanResult.inl"
