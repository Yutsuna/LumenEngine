/**
 * @file SceneExtractor.hpp
 * @brief Autonomous extraction of complete, canonical rendering snapshots.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>
#include <SceneExport.hpp>

#include <Memory/Span.hpp>
#include <Rendering/RenderRecords.hpp>
#include <Rendering/RenderSnapshotTypes.hpp>
#include <Templates/Expected.hpp>

namespace Lumen
{

class FRenderSnapshotStream;

/**
 * @struct FSceneExtractionInput
 * @brief Non-owning committed Scene state consumed by one publication.
 * @details Proxy IDs must be nonzero and unique within each record family. Each mesh TransformIndex addresses the
 *          source transform span and must reference the same ProxyId as the mesh; extraction remaps it after sorting.
 */
struct SCENE_EXPORT FSceneExtractionInput final
{
    UInt64 WorldId    = 0ULL;
    UInt64 FrameIndex = 0ULL;
    TSpan<const FRenderTransformRecord> Transforms{};
    TSpan<const FRenderMeshRecord> Meshes{};
    TSpan<const FRenderCameraRecord> Cameras{};
    TSpan<const FRenderLightRecord> Lights{};
};

/**
 * @class FSceneExtractor
 * @brief Validates and publishes a complete snapshot from autonomous spans, without depending on FWorld.
 * @details One extractor owns the write side of one stream. A capacity or source-index failure is returned before the
 *          writer slot is modified. Successful publication performs no allocation within the reserved capacities.
 */
class SCENE_EXPORT FSceneExtractor final : public FNonCopyable, public FNonMovable
{
public:

    explicit FSceneExtractor ( FRenderSnapshotStream &InStream ) noexcept;
    ~FSceneExtractor () noexcept = default;

    /**
     * @brief Publishes all input records, sorted by ascending proxy ID in each family.
     * @return The published header, or a structured error with the previous published snapshot left intact.
     */
    [[nodiscard]] TExpected<FRenderSnapshotHeader, FRenderSnapshotError>
    Publish ( const FSceneExtractionInput &InInput ) noexcept;

private:

    FRenderSnapshotStream *Stream = nullptr;
    UInt64 NextSnapshotSequence   = 1ULL;
};

} // namespace Lumen
