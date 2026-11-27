/**
 * @file RenderProxyTypes.hpp
 * @brief Value contracts for the frame-level render proxy registry.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/CoreTypes.hpp>

#include <Rendering/RenderIds.hpp>
#include <Rendering/RenderRecords.hpp>

namespace Lumen
{

/**
 * @enum ERenderProxyError
 * @brief Failures reported while synchronizing a proxy snapshot.
 */
enum class ERenderProxyError : UInt8
{
    InvalidDesc,
    CapacityExceeded,
};

/**
 * @struct FRenderProxyEntry
 * @brief One canonical mesh proxy owned by the render-side registry.
 */
struct FRenderProxyEntry final
{
    FRenderProxyId ProxyId;
    FRenderAssetId MeshAssetId;
    FRenderAssetId MaterialAssetId;
    USize TransformIndex    = 0ULL;
    ERenderMeshFlags Flags  = ERenderMeshFlags::None;
    UInt64 LastSyncSequence = 0ULL;
};

/**
 * @struct FRenderProxyStats
 * @brief Per-snapshot counters describing how the proxy set changed.
 */
struct FRenderProxyStats final
{
    UInt32 Created  = 0U;
    UInt32 Updated  = 0U;
    UInt32 Removed  = 0U;
    UInt32 Rejected = 0U;
};

} // namespace Lumen
