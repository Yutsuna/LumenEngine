/**
 * @file RHISubresourceRange.inl
 * @brief Inline implementation of RHISubresourceRange.
 */

#pragma once

#include <RHI/Resources/RHISubresourceRange.hpp>

constexpr Lumen::Bool Lumen::RHI::FRHISubresourceRange::IsValid () const noexcept
{
    return MipCount > 0U and LayerCount > 0U;
}
