/**
 * @file RenderSnapshotStorage.hpp
 * @brief Owning storage reserved independently for each snapshot stream slot.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>
#include <SceneExport.hpp>

#include <Rendering/RenderSnapshotView.hpp>

#include <Templates/Vector.hpp>

namespace Lumen
{

class FRenderSnapshotStream;
class FSceneExtractor;

/**
 * @class FRenderSnapshotStorage
 * @brief Owns the record arrays behind one slot; consumers only receive FRenderSnapshotView.
 * @details This is Scene implementation surface made visible for stream layout. Records cannot be accessed mutably
 *          through the rendering consumer contract, and storage never shares actor or asset ownership.
 */
class SCENE_EXPORT FRenderSnapshotStorage final : public FNonCopyable, public FNonMovable
{
public:

    FRenderSnapshotStorage () noexcept  = default;
    ~FRenderSnapshotStorage () noexcept = default;

private:

    void Reserve ( const FRenderSnapshotCapacities &InCapacities ) noexcept;
    void Reset () noexcept;
    [[nodiscard]] FRenderSnapshotView GetView () const noexcept;

private:

    FRenderSnapshotHeader Header{};
    TVector<FRenderTransformRecord> Transforms;
    TVector<FRenderMeshRecord> Meshes;
    TVector<FRenderCameraRecord> Cameras;
    TVector<FRenderLightRecord> Lights;

    friend class FRenderSnapshotStream;
    friend class FSceneExtractor;
};

} // namespace Lumen
