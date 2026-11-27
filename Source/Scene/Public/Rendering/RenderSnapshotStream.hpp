/**
 * @file RenderSnapshotStream.hpp
 * @brief Latest-only, single-reader transport for complete rendering snapshots.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>
#include <SceneExport.hpp>

#include <Rendering/RenderSnapshotStorage.hpp>

#include <Thread/TripleBuffer.hpp>

namespace Lumen
{

class FSceneExtractor;

/**
 * @class FRenderSnapshotStream
 * @brief Owns three pre-reserved slots and exposes only the latest published complete snapshot.
 * @details Exactly one FSceneExtractor may publish, and exactly one reader thread may call SwapReadBuffers.
 *          A view remains stable until the next successful reader swap or stream destruction.
 */
class SCENE_EXPORT FRenderSnapshotStream final : public FNonCopyable, public FNonMovable
{
public:

    FRenderSnapshotStream () noexcept;
    explicit FRenderSnapshotStream ( const FRenderSnapshotCapacities &InCapacities ) noexcept;
    ~FRenderSnapshotStream () noexcept = default;

public:

    /**
     * @brief Promotes the newest pending publication, coalescing any intermediate snapshots.
     * @return True when a new slot was promoted; false leaves the current view untouched.
     */
    Bool SwapReadBuffers () noexcept;

    /** @return An immutable view expiring at the next successful reader swap. */
    [[nodiscard]] FRenderSnapshotView GetReadSnapshot () const noexcept;

    /** @return True when at least one publication has not been consumed. */
    [[nodiscard]] Bool HasPendingSnapshot () const noexcept;

    /** @return The fixed capacities reserved in all three slots. */
    [[nodiscard]] FRenderSnapshotCapacities GetCapacities () const noexcept;

    /** @return Sequence of the newest successful publication, or zero before the first one. */
    [[nodiscard]] UInt64 GetLastPublishedSequence () const noexcept;

    /** @return Sequence most recently promoted by the reader, or zero before the first swap. */
    [[nodiscard]] UInt64 GetLastConsumedSequence () const noexcept;

    /** @return Number of complete snapshots coalesced without ever being consumed. */
    [[nodiscard]] UInt64 GetDroppedSnapshotCount () const noexcept;

private:

    FRenderSnapshotStorage Slots[3];
    Thread::TTripleBuffer<FRenderSnapshotStorage> SnapshotBuffer;
    FRenderSnapshotCapacities Capacities{};
    TAtomic<UInt64> LastPublishedSequence{ 0ULL };
    TAtomic<UInt64> LastConsumedSequence{ 0ULL };
    TAtomic<UInt64> DroppedSnapshotCount{ 0ULL };

    friend class FSceneExtractor;
};

} // namespace Lumen
