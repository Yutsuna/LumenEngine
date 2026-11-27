/**
 * @file RenderSnapshotTypes.hpp
 * @brief Snapshot metadata, reserved capacities and structured extraction errors.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <SceneExport.hpp>

#include <Concepts/TriviallyCopyable.hpp>

namespace Lumen
{

/** @struct FRenderSnapshotCapacities @brief Fixed record capacities reserved in every stream slot at initialization. */
struct SCENE_EXPORT FRenderSnapshotCapacities final
{
    USize TransformCapacity = 0ULL;
    USize MeshCapacity      = 0ULL;
    USize CameraCapacity    = 0ULL;
    USize LightCapacity     = 0ULL;

    constexpr Bool operator==( const FRenderSnapshotCapacities & ) const noexcept = default;
};

/** @struct FRenderSnapshotHeader @brief Monotonic frame metadata and exact record counts for one complete snapshot. */
struct SCENE_EXPORT FRenderSnapshotHeader final
{
    UInt64 WorldId          = 0ULL;
    UInt64 FrameIndex       = 0ULL;
    UInt64 SnapshotSequence = 0ULL;
    USize TransformCount    = 0ULL;
    USize MeshCount         = 0ULL;
    USize CameraCount       = 0ULL;
    USize LightCount        = 0ULL;
    FRenderSnapshotCapacities Capacities{};
};

/**
 * @enum ERenderSnapshotRecordType
 * @brief Record family associated with an extraction error.
 */
enum class ERenderSnapshotRecordType : UInt8
{
    None,
    Transform,
    Mesh,
    Camera,
    Light,
};

/**
 * @enum ERenderSnapshotErrorCode
 * @brief Failures rejected before a new snapshot can be published.
 */
enum class ERenderSnapshotErrorCode : UInt8
{
    None,
    CapacityExceeded,
    InvalidTransformIndex,
    TransformProxyMismatch,
};

/**
 * @struct FRenderSnapshotError
 * @brief Describes a rejected publication; RequiredCount and Capacity are populated for capacity failures.
 */
struct SCENE_EXPORT FRenderSnapshotError final
{
    ERenderSnapshotErrorCode Code        = ERenderSnapshotErrorCode::None;
    ERenderSnapshotRecordType RecordType = ERenderSnapshotRecordType::None;
    USize RecordIndex                    = 0ULL;
    USize RequiredCount                  = 0ULL;
    USize Capacity                       = 0ULL;

    constexpr Bool operator==( const FRenderSnapshotError & ) const noexcept = default;
};

static_assert( Concepts::CTriviallyCopyable<FRenderSnapshotCapacities> );
static_assert( Concepts::CTriviallyCopyable<FRenderSnapshotHeader> );
static_assert( Concepts::CTriviallyCopyable<FRenderSnapshotError> );

} // namespace Lumen
