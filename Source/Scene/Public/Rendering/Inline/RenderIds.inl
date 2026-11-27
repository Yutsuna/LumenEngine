/**
 * @file RenderIds.inl
 * @brief Inline implementation of rendering identifiers.
 */

#pragma once

#include <Rendering/RenderIds.hpp>

constexpr Lumen::FRenderProxyId::FRenderProxyId ( const UInt64 InValue ) noexcept : Value( InValue )
{
    /** Empty */
}

constexpr Lumen::Bool Lumen::FRenderProxyId::IsValid () const noexcept
{
    return Value != 0ULL;
}

constexpr Lumen::UInt64 Lumen::FRenderProxyId::GetValue () const noexcept
{
    return Value;
}

constexpr Lumen::FRenderAssetId::FRenderAssetId ( const UInt64 InValue ) noexcept : Value( InValue )
{
    /** Empty */
}

constexpr Lumen::Bool Lumen::FRenderAssetId::IsValid () const noexcept
{
    return Value != 0ULL;
}

constexpr Lumen::UInt64 Lumen::FRenderAssetId::GetValue () const noexcept
{
    return Value;
}
