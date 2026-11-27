/**
 * @file RenderRecords.hpp
 * @brief Trivial records consumed through immutable rendering snapshots.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/EnumFlags.hpp>
#include <SceneExport.hpp>

#include <Concepts/TriviallyCopyable.hpp>

#include <Maths/Matrix.hpp>
#include <Maths/Vec.hpp>

#include <Rendering/RenderIds.hpp>

namespace Lumen
{

/**
 * @enum ERenderTransformFlags
 * @brief Visibility state attached to a world transform.
 */
enum class ERenderTransformFlags : UInt32 /** NOLINT */
{
    None    = 0U,
    Visible = 1U << 0U,
};

LUMEN_ENUM_FLAGS( ERenderTransformFlags );

/**
 * @enum ERenderMeshFlags
 * @brief Visibility and shadow state attached to a mesh proxy.
 */
enum class ERenderMeshFlags : UInt32 /** NOLINT */
{
    None        = 0U,
    Visible     = 1U << 0U,
    CastsShadow = 1U << 1U,
};

LUMEN_ENUM_FLAGS( ERenderMeshFlags );

/**
 * @enum ERenderCameraFlags
 * @brief Camera selection state; the active camera with the lowest proxy ID wins.
 */
enum class ERenderCameraFlags : UInt32 /** NOLINT */
{
    None   = 0U,
    Active = 1U << 0U,
};

LUMEN_ENUM_FLAGS( ERenderCameraFlags );

/**
 * @enum ERenderLightFlags
 * @brief Visibility state attached to a light proxy.
 */
enum class ERenderLightFlags : UInt32 /** NOLINT */
{
    None    = 0U,
    Visible = 1U << 0U,
};

LUMEN_ENUM_FLAGS( ERenderLightFlags );

/**
 * @enum ERenderLightType
 * @brief Minimal light shapes represented by the extraction contract.
 */
enum class ERenderLightType : UInt8
{
    Directional,
    Point,
    Spot,
};

/** @struct FRenderBounds @brief World-space axis-aligned bounds. */
struct SCENE_EXPORT FRenderBounds final
{
    Maths::FVec3f Min;
    Maths::FVec3f Max;
};

/** @struct FRenderViewport @brief Camera viewport and depth range in normalized depth units. */
struct SCENE_EXPORT FRenderViewport final
{
    Float32 X        = 0.0F;
    Float32 Y        = 0.0F;
    Float32 Width    = 0.0F;
    Float32 Height   = 0.0F;
    Float32 MinDepth = 0.0F;
    Float32 MaxDepth = 1.0F;
};

/** @struct FRenderTransformRecord @brief World transform, bounds and visibility for one render proxy. */
struct SCENE_EXPORT FRenderTransformRecord final
{
    FRenderProxyId ProxyId;
    Maths::FMat4x4f WorldTransform = Maths::FMat4x4f::Identity();
    FRenderBounds Bounds{};
    ERenderTransformFlags Flags = ERenderTransformFlags::Visible;
};

/** @struct FRenderMeshRecord @brief Content references and canonical transform index for one mesh proxy. */
struct SCENE_EXPORT FRenderMeshRecord final
{
    FRenderProxyId ProxyId;
    FRenderAssetId MeshAssetId;
    FRenderAssetId MaterialAssetId;
    USize TransformIndex   = 0ULL;
    ERenderMeshFlags Flags = ERenderMeshFlags::Visible;
};

/** @struct FRenderCameraRecord @brief Camera matrices, clipping planes and viewport for one camera proxy. */
struct SCENE_EXPORT FRenderCameraRecord final
{
    FRenderProxyId ProxyId;
    Maths::FMat4x4f View       = Maths::FMat4x4f::Identity();
    Maths::FMat4x4f Projection = Maths::FMat4x4f::Identity();
    Float32 NearPlane          = 0.1F;
    Float32 FarPlane           = 1000.0F;
    FRenderViewport Viewport{};
    ERenderCameraFlags Flags = ERenderCameraFlags::None;
};

/** @struct FRenderLightRecord @brief Minimal POD light description; it does not promise a complete lighting model. */
struct SCENE_EXPORT FRenderLightRecord final
{
    FRenderProxyId ProxyId;
    ERenderLightType Type   = ERenderLightType::Directional;
    Maths::FVec3f Color     = { 1.0F };
    Float32 Intensity       = 1.0F;
    Maths::FVec3f Direction = { 0.0F, 0.0F, -1.0F };
    Maths::FVec3f Position;
    Float32 Range           = 0.0F;
    Float32 InnerConeAngle  = 0.0F;
    Float32 OuterConeAngle  = 0.0F;
    ERenderLightFlags Flags = ERenderLightFlags::Visible;
};

static_assert( Concepts::CTriviallyCopyable<Maths::FMat4x4f>, "Snapshot matrices must be trivially copyable." );
static_assert( Concepts::CTriviallyCopyable<Maths::FVec3f>, "Snapshot vectors must be trivially copyable." );
static_assert( Concepts::CTriviallyCopyable<FRenderBounds> );
static_assert( Concepts::CTriviallyCopyable<FRenderViewport> );
static_assert( Concepts::CTriviallyCopyable<FRenderTransformRecord> );
static_assert( Concepts::CTriviallyCopyable<FRenderMeshRecord> );
static_assert( Concepts::CTriviallyCopyable<FRenderCameraRecord> );
static_assert( Concepts::CTriviallyCopyable<FRenderLightRecord> );

} // namespace Lumen
