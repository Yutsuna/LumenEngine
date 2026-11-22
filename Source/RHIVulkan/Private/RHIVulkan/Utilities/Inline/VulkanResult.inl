/**
 * @file VulkanResult.inl
 * @brief Inline implementation of Vulkan result code translation.
 */

#pragma once

#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <vulkan/vulkan_core.h>

constexpr Lumen::RHI::ERHIErrorCode Lumen::RHIVulkan::TranslateVulkanResultCode ( VkResult InResult,
                                                                                  Bool bNotReadyAsTimeout ) noexcept
{
    if ( InResult == VK_NOT_READY )
    {
        return bNotReadyAsTimeout ? RHI::ERHIErrorCode::Timeout : RHI::ERHIErrorCode::InvalidState;
    }

    switch ( InResult )
    {
#define LUMEN_VK_TRANSLATE_CASE( VkName, ErrCode )                                                                               \
    case VkName:                                                                                                                 \
        return RHI::ERHIErrorCode::ErrCode;
        LUMEN_VK_ERROR_RESULT_LIST( LUMEN_VK_TRANSLATE_CASE )
#undef LUMEN_VK_TRANSLATE_CASE
    default:
        return RHI::ERHIErrorCode::BackendFailure;
    }
}

constexpr Lumen::Bool Lumen::RHIVulkan::IsVulkanResultSuccess ( VkResult InResult ) noexcept
{
    return InResult == VK_SUCCESS;
}
