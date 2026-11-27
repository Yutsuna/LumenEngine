/**
 * @file RenderSnapshotView.hpp
 * @brief Immutable, non-owning view of one complete rendering snapshot.
 */

#pragma once

#include <SceneExport.hpp>

#include <Memory/Span.hpp>

#include <Rendering/RenderRecords.hpp>
#include <Rendering/RenderSnapshotTypes.hpp>

namespace Lumen
{

/**
 * @struct FRenderSnapshotView
 * @brief Exposes const spans over one complete, canonically sorted snapshot.
 * @details The header is copied into the view. The spans expire on the reader's next successful SwapReadBuffers call
 *          and must never be retained by render-graph or RHI command recording beyond that point.
 */
struct SCENE_EXPORT FRenderSnapshotView final
{
    FRenderSnapshotHeader Header{};
    TSpan<const FRenderTransformRecord> Transforms{};
    TSpan<const FRenderMeshRecord> Meshes{};
    TSpan<const FRenderCameraRecord> Cameras{};
    TSpan<const FRenderLightRecord> Lights{};
};

} // namespace Lumen
