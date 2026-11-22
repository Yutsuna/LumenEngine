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

template <class TProperty, auto Function, class... TArguments>
Lumen::TExpected<Lumen::TVector<TProperty>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::Internal::EnumerateVulkanItems ( FStringView InIncompleteMessage, TArguments... InArguments ) noexcept
{
    constexpr UInt32 MaxEnumerationRetries = 4U;

    for ( UInt32 Attempt = 0U; Attempt < MaxEnumerationRetries; ++Attempt )
    {
        UInt32 Count = 0U;
        LUMEN_VK_TRY( Function( InArguments..., &Count, nullptr ), RHI::ERHIErrorCondition::Device );

        TVector<TProperty> Properties( Count );
        if ( Count == 0U )
        {
            return Properties;
        }

        const VkResult ListResult = Function( InArguments..., &Count, Properties.data() );
        if ( ListResult == VK_SUCCESS )
        {
            Properties.resize( Count );
            return Properties;
        }
        if ( ListResult != VK_INCOMPLETE )
        {
            LUMEN_VK_TRY( ListResult, RHI::ERHIErrorCondition::Device );
        }
    }

    return MakeUnexpected( RHI::MakeRHICorrupted( RHI::ERHIErrorCondition::Device, InIncompleteMessage ) );
}
