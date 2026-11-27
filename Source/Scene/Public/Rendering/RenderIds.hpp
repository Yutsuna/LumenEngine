/**
 * @file RenderIds.hpp
 * @brief Strong, non-owning identifiers crossing the Scene rendering boundary.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <SceneExport.hpp>

#include <Concepts/TriviallyCopyable.hpp>

#include <compare>

namespace Lumen
{

/**
 * @struct FRenderProxyId
 * @brief Identifies a render proxy without owning its Scene or renderer lifetime.
 */
struct SCENE_EXPORT FRenderProxyId final
{
    constexpr FRenderProxyId () noexcept = default;
    constexpr explicit FRenderProxyId ( UInt64 InValue ) noexcept;

    [[nodiscard]] constexpr Bool IsValid () const noexcept;
    [[nodiscard]] constexpr UInt64 GetValue () const noexcept;

    constexpr Bool operator==( const FRenderProxyId & ) const noexcept  = default;
    constexpr auto operator<=>( const FRenderProxyId & ) const noexcept = default;

private:

    UInt64 Value = 0ULL;
};

/**
 * @struct FRenderAssetId
 * @brief Identifies content supplied by the asset layer, never a native or RHI resource.
 */
struct SCENE_EXPORT FRenderAssetId final
{
    constexpr FRenderAssetId () noexcept = default;
    constexpr explicit FRenderAssetId ( UInt64 InValue ) noexcept;

    [[nodiscard]] constexpr Bool IsValid () const noexcept;
    [[nodiscard]] constexpr UInt64 GetValue () const noexcept;

    constexpr Bool operator==( const FRenderAssetId & ) const noexcept  = default;
    constexpr auto operator<=>( const FRenderAssetId & ) const noexcept = default;

private:

    UInt64 Value = 0ULL;
};

inline constexpr FRenderProxyId InvalidRenderProxyId{};
inline constexpr FRenderAssetId InvalidRenderAssetId{};

static_assert( Concepts::CTriviallyCopyable<FRenderProxyId>, "Render proxy IDs must remain trivial values." );
static_assert( Concepts::CTriviallyCopyable<FRenderAssetId>, "Render asset IDs must remain trivial values." );
static_assert( sizeof( FRenderProxyId ) == sizeof( UInt64 ), "Render proxy IDs must not carry ownership or auxiliary state." );
static_assert( sizeof( FRenderAssetId ) == sizeof( UInt64 ), "Render asset IDs must not carry ownership or auxiliary state." );

} // namespace Lumen

#include "Inline/RenderIds.inl"
