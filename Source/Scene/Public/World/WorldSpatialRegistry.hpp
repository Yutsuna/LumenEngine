/**
 * @file WorldSpatialRegistry.hpp
 * @brief Data-Oriented registry for spatial and renderable data.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>
#include <SceneExport.hpp>

#include <Actor/ActorTypes.hpp>

#include <Memory/Span.hpp>

#include <Templates/AtomicBitset.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Map.hpp>
#include <Templates/Vector.hpp>

#include <HAL/PlatformMutex.hpp>
#include <Maths/Matrix.hpp>

#include <Thread/TripleBuffer.hpp>

namespace Lumen
{

/**
 * @enum ESpatialComponentType
 * @brief Categorizes registry data for granular dirty tracking.
 */
enum class ESpatialComponentType : UInt8
{
    Transform,
    Mesh,
    Shader,
    Count
};

/** @enum ESpatialRegistryError @brief Structured failures rejected before a spatial entity is registered. */
enum class ESpatialRegistryError : UInt8
{
    CapacityExceeded,
};

/** @struct FSpatialRegistryCapacities @brief Fixed entity capacities reserved in all three snapshot slots. */
struct SCENE_EXPORT FSpatialRegistryCapacities final
{
    USize EntityCapacity = 0ULL;

    constexpr Bool operator==( const FSpatialRegistryCapacities & ) const noexcept = default;
};

/**
 * @struct FSpatialSnapshotView
 * @brief Immutable view of one published spatial snapshot.
 * @details Spans expire at the reader's next successful SwapReadBuffers call and must never be retained beyond that point.
 */
struct SCENE_EXPORT FSpatialSnapshotView final
{
    TSpan<const ActorID> EntityIDs;
    TSpan<const Maths::FMat4x4f> Transforms;
};

/**
 * @struct FSpatialRegistryStorage
 * @brief Owning, pre-reserved storage behind one spatial snapshot slot.
 * @details This is registry implementation surface made visible for slot layout; consumers only receive FSpatialSnapshotView.
 */
struct SCENE_EXPORT FSpatialRegistryStorage final : public FNonCopyable, public FNonMovable
{
    FSpatialRegistryStorage () noexcept  = default;
    ~FSpatialRegistryStorage () noexcept = default;

private:

    void Reserve ( const FSpatialRegistryCapacities &InCapacities ) noexcept;
    void Reset () noexcept;
    [[nodiscard]] FSpatialSnapshotView GetView () const noexcept;

private:

    TVector<ActorID> EntityIDs;
    TVector<Maths::FMat4x4f> Transforms;

    friend class FSpatialRegistry;
};

/**
 * @class FSpatialRegistry
 * @brief High-performance bridge between Actors and the Renderer.
 */
class SCENE_EXPORT FSpatialRegistry final : public FNonCopyable
{
public:

    /** @brief Legacy singleton with world-default capacity, for callers not yet migrated to an injected registry. */
    FSpatialRegistry () noexcept;

    /** @brief Creates a registry whose three snapshot slots are reserved for InCapacities. */
    explicit FSpatialRegistry ( const FSpatialRegistryCapacities &InCapacities ) noexcept;

    /** @brief Legacy global access; new owners should hold their own FSpatialRegistry instance. */
    static FSpatialRegistry &Get () noexcept;

public:

    /** @brief Legacy infallible adapter; registrations rejected by capacity are recorded and dropped. */
    void RegisterSpatialEntity ( ActorID InId ) noexcept;

    /** @brief Registers a spatial entity or returns a structured capacity failure. */
    [[nodiscard]] TExpected<void, ESpatialRegistryError> TryRegisterSpatialEntity ( ActorID InId ) noexcept;

    /** @brief Unregisters a spatial entity from the registry. */
    void UnregisterSpatialEntity ( ActorID InId ) noexcept;

    /**
     * @brief Updates the transform of a spatial entity. (Thread-safe)
     * @param InId The ID of the entity to update.
     * @param InTransform The new transform.
     */
    void UpdateTransform ( ActorID InId, const Maths::FMat4x4f &InTransform ) noexcept;

    /** @brief Publishes the current state of the registry without allocating. */
    void Publish () noexcept;

    /** @brief Swaps the read buffers. */
    void SwapReadBuffers () noexcept;

    /** @brief Gets an immutable view of the current published snapshot. */
    [[nodiscard]] FSpatialSnapshotView GetReadSnapshot () const noexcept;

    /** @return The capacities reserved in all three slots. */
    [[nodiscard]] FSpatialRegistryCapacities GetCapacities () const noexcept;

    /** @return Sequence of the newest publication, or zero before the first one. */
    [[nodiscard]] UInt64 GetLastPublishedSequence () const noexcept;

    /** @return Sequence most recently promoted by the reader, or zero before the first swap. */
    [[nodiscard]] UInt64 GetLastConsumedSequence () const noexcept;

    /** @return Number of publications coalesced without ever being consumed. */
    [[nodiscard]] UInt64 GetDroppedSnapshotCount () const noexcept;

    /** @return Number of registrations rejected because the reserved capacity was exhausted. */
    [[nodiscard]] UInt64 GetDroppedRegistrationCount () const noexcept;

private:

    /** @brief Mark a component dirty across all TripleBuffer slots */
    void MarkComponentDirty ( USize InIndex, ESpatialComponentType InComponent ) noexcept;

    /** @brief Resizes the delta trackers and flags structural changes across all TripleBuffer slots */
    void ResizeTrackersAndFlagStructuralChange () noexcept;

    struct FWorkingData
    {
        TVector<ActorID> EntityIDs;
        TVector<Maths::FMat4x4f> Transforms;
        TMap<ActorID, USize> IDToIndex;

        using FMapIterator = TMap<ActorID, USize>::const_iterator;
    };

    /**
     * @struct FDeltaTracker
     * @brief Tracks which components have changed for each entity to optimize data copying during Publish.
     */
    struct FDeltaTracker
    {
        /** One bitset per component to avoid over-copying unrelated data */
        FAtomicBitset ComponentDirtyStates[static_cast<USize>( ESpatialComponentType::Count )];
        Bool bMetadataDirty = true;

        /** @brief Resizes the delta tracker. */
        void Resize ( USize InNumBits );

        /** @brief Clears all dirty flags. */
        void ClearAll () noexcept;
    };

private:

    mutable HAL::FPlatformSharedMutex RegistryMutex;
    FSpatialRegistryCapacities Capacities;
    FWorkingData WorkingData;
    FSpatialRegistryStorage Slots[Thread::TripleBufferCount];
    Thread::TTripleBuffer<FSpatialRegistryStorage> SnapshotBuffer;
    FDeltaTracker DeltaTrackers[Thread::TripleBufferCount];

    /** @brief One bitset per component. UpdateTransform marks here (1 atomic RMW). */
    FAtomicBitset Pending[static_cast<USize>( ESpatialComponentType::Count )];

    TAtomic<UInt64> LastPublishedSequence{ 0ULL };
    TAtomic<UInt64> LastConsumedSequence{ 0ULL };
    TAtomic<UInt64> DroppedSnapshotCount{ 0ULL };
    TAtomic<UInt64> DroppedRegistrationCount{ 0ULL };
};

} // namespace Lumen
