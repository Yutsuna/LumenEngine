/**
 * @file VulkanResult.cpp
 * @brief Implementation of Vulkan result code mapping utilities.
 */

#include <RHIVulkan/Utilities/VulkanResult.hpp>

Lumen::RHI::FRHIError Lumen::RHIVulkan::MapVulkanResult ( VkResult InResult,
                                                          RHI::ERHIErrorCondition InCondition,
                                                          FStringView InOperation,
                                                          Bool bNotReadyAsTimeout ) noexcept
{
    const RHI::ERHIErrorCode Code = TranslateVulkanResultCode( InResult, bNotReadyAsTimeout );
    const FStringView ResultName  = LUMEN_VK_RESULT_STR( InResult );
    RHI::FRHIError Error          = { Code, InCondition, ResultName, TOptional<Int64>( static_cast<Int64>( InResult ) ) };

    if ( InOperation.empty() )
    {
        return Error;
    }
    return Error.WithContext( InOperation );
}
