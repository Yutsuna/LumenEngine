/**
 * @file WorldRenderContent.hpp
 * @brief Control-time authored rendering content owned by a world.
 */

#pragma once

#include <SceneExport.hpp>

#include <Rendering/RenderRecords.hpp>

#include <Templates/Vector.hpp>

namespace Lumen
{

/**
 * @struct FWorldRenderContent
 * @brief Non-actor POD records supplied during control operations and merged into the next extraction.
 * @details Transform records are produced by the world from committed spatial state, so only the other three
 *          families are authored here. Mesh ProxyIds select the matching committed transform; TransformIndex is
 *          rewritten by the world and need not be set by the caller. Records are owned, pre-reserved, and never
 *          expose an AActor pointer or a renderer resource.
 */
struct SCENE_EXPORT FWorldRenderContent final
{
    TVector<FRenderMeshRecord> Meshes;
    TVector<FRenderCameraRecord> Cameras;
    TVector<FRenderLightRecord> Lights;
};

} // namespace Lumen
