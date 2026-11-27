/**
 * @file SceneHierarchy.hpp
 * @brief Bounded actor hierarchy with deterministic world-transform publication.
 */

#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorTypes.hpp>

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Maths/Matrix.hpp>

#include <Memory/Span.hpp>
#include <Memory/UniquePtr.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

/**
 * @enum ESceneHierarchyError
 * @brief Structured failures reported by scene-hierarchy operations.
 */
enum class ESceneHierarchyError : UInt8
{
    InvalidCapacity,
    AlreadyInitialized,
    NotInitialized,
    OutOfMemory,
    InvalidActor,
    AlreadyRegistered,
    UnknownActor,
    UnknownParent,
    SelfParent,
    CycleDetected,
    CapacityExceeded,
    StaleParent,
    DeferredQueueFull,
};

/**
 * @struct FSceneHierarchyTransformRecord
 * @brief Value record consumed by scene extraction after publication.
 */
struct SCENE_EXPORT FSceneHierarchyTransformRecord
{
    ActorID ActorId                = InvalidActorID;
    Maths::FMat4x4f WorldTransform = Maths::FMat4x4f::Identity();
};

/**
 * @struct FSceneHierarchyUpdateResult
 * @brief Observable work performed by one explicit publication.
 */
struct SCENE_EXPORT FSceneHierarchyUpdateResult
{
    USize UpdatedTransformCount   = 0ULL;
    USize PublishedTransformCount = 0ULL;
    UInt64 PublicationSequence    = 0ULL;
};

/**
 * @class FSceneHierarchy
 * @brief Owns spatial relations between weak actor IDs, never actor lifetimes.
 * @details Initialize reserves every node and publication record up front. Mutations and publication are serial game-phase
 *          operations; the class performs no internal synchronization. World composition is ParentWorld * Local using Lumen's
 *          column-major matrices. The published span is immutable until the next Publish or UpdateWorld call and is always
 *          ordered by ActorID, independently of registration order and hash/container addresses.
 */
class SCENE_EXPORT FSceneHierarchy final : public FNonCopyable, public FNonMovable
{
public:

    /** @brief Creates an uninitialized hierarchy without allocating storage. */
    FSceneHierarchy () noexcept;

    /** @brief Releases the preallocated node and publication storage. */
    ~FSceneHierarchy () noexcept;

public:

    /**
     * @brief Allocates fixed storage for InCapacity nodes and their published records.
     * @return Success, or InvalidCapacity, AlreadyInitialized or OutOfMemory without changing the previous state.
     */
    [[nodiscard]] TExpected<void, ESceneHierarchyError> Initialize ( USize InCapacity ) noexcept;

    /** @brief Registers a root node with a local transform, or fails without partial insertion. */
    [[nodiscard]] TExpected<void, ESceneHierarchyError>
    Register ( ActorID InActorId, const Maths::FMat4x4f &InLocalTransform = Maths::FMat4x4f::Identity() ) noexcept;

    /**
     * @brief Unregisters one node without destroying any actor or descendant.
     * @details Direct children become roots and preserve their current world transforms as their new local transforms.
     *          Descendants below those children keep their existing local transforms. Removal is published on the next update.
     */
    [[nodiscard]] TExpected<void, ESceneHierarchyError> Unregister ( ActorID InActorId ) noexcept;

    /**
     * @brief Reparents a node while preserving its local transform.
     * @details InvalidActorID as parent is equivalent to Detach. Unknown parents, self-parenting and ancestry cycles are rejected
     *          before any link or transform is modified.
     */
    [[nodiscard]] TExpected<void, ESceneHierarchyError> Attach ( ActorID InActorId, ActorID InParentId ) noexcept;

    /**
     * @brief Makes a node a root and preserves its current world transform as its new local transform.
     * @details Pending local and structural changes are resolved in working storage before detaching, without publishing them.
     */
    [[nodiscard]] TExpected<void, ESceneHierarchyError> Detach ( ActorID InActorId ) noexcept;

    /** @brief Changes a registered node's local transform and defers descendant world updates. */
    [[nodiscard]] TExpected<void, ESceneHierarchyError> SetLocalTransform ( ActorID InActorId,
                                                                            const Maths::FMat4x4f &InLocalTransform ) noexcept;

    /**
     * @brief Resolves dirty world transforms parent-first and publishes a complete ActorID-ordered view.
     * @return Recomputed transform count, complete published count and the incremented publication sequence.
     */
    [[nodiscard]] TExpected<FSceneHierarchyUpdateResult, ESceneHierarchyError> UpdateWorld () noexcept;

    /** @brief Synonym of UpdateWorld for deferred Scene publication pipelines. */
    [[nodiscard]] TExpected<FSceneHierarchyUpdateResult, ESceneHierarchyError> Publish () noexcept;

public:

    /** @return The last computed working world transform, or an empty optional for an unknown actor. */
    [[nodiscard]] TOptional<Maths::FMat4x4f> GetWorldTransform ( ActorID InActorId ) const noexcept;

    /** @return The current local transform, or an empty optional for an unknown actor. */
    [[nodiscard]] TOptional<Maths::FMat4x4f> GetLocalTransform ( ActorID InActorId ) const noexcept;

    /** @return The parent ID, including InvalidActorID for a root, or an empty optional for an unknown actor. */
    [[nodiscard]] TOptional<ActorID> GetParentId ( ActorID InActorId ) const noexcept;

    /** @return Whether the actor is registered in this hierarchy. */
    [[nodiscard]] Bool Contains ( ActorID InActorId ) const noexcept;

    /** @return Whether Initialize succeeded. */
    [[nodiscard]] Bool IsInitialized () const noexcept;

    /** @return The fixed node capacity. */
    [[nodiscard]] USize GetCapacity () const noexcept;

    /** @return The current working node count, including changes not yet published. */
    [[nodiscard]] USize GetNodeCount () const noexcept;

    /** @return The last explicit publication sequence. */
    [[nodiscard]] UInt64 GetPublicationSequence () const noexcept;

    /** @return A complete, immutable-until-next-publication view sorted by ascending ActorID. */
    [[nodiscard]] TSpan<const FSceneHierarchyTransformRecord> GetPublishedTransforms () const noexcept;

    /** @return Working depth of a registered node, or an empty optional for an unknown actor. */
    [[nodiscard]] TOptional<USize> GetDepth ( ActorID InActorId ) const noexcept;

    /**
     * @brief Level boundaries into the level-ordered transform view.
     * @details Entry D is the first record at depth D; entry D+1 is the first record at depth D+1. The span has Capacity+1
     *          entries after Initialize, so depth Capacity-1 is addressable. Records inside one level are ActorID ordered.
     */
    [[nodiscard]] TSpan<const USize> GetLevelStarts () const noexcept;

    /** @return Published records grouped by depth, then sorted by ActorID; empty until the first UpdateWorld. */
    [[nodiscard]] TSpan<const FSceneHierarchyTransformRecord> GetLevelOrderedTransforms () const noexcept;

    /**
     * @brief Visits every published record at one depth in canonical ActorID order.
     * @tparam TVisitor Callable taking `const FSceneHierarchyTransformRecord &`; forwarded without type erasure or allocation.
     */
    template <typename TVisitor> void ForEachLevel ( USize InDepth, TVisitor &&InVisitor ) const noexcept;

private:

    struct FNode;

    [[nodiscard]] USize LowerBound ( ActorID InActorId ) const noexcept;
    [[nodiscard]] USize FindNodeIndex ( ActorID InActorId ) const noexcept;

    void LinkNode ( FNode &InNode ) noexcept;
    void UnlinkNode ( FNode &InNode ) noexcept;
    void MarkDirty ( FNode &InNode ) noexcept;
    [[nodiscard]] USize UpdateWorldTransforms () noexcept;
    void RebuildLevelOrder () noexcept;

private:

    TUniquePtr<FNode[]> Nodes;
    TUniquePtr<FSceneHierarchyTransformRecord[]> PublishedTransforms;
    TUniquePtr<FSceneHierarchyTransformRecord[]> LevelOrderedTransforms;
    TUniquePtr<USize[]> LevelStarts;
    TUniquePtr<USize[]> LevelCursor;

    USize Capacity                = 0ULL;
    USize NodeCount               = 0ULL;
    USize PublishedTransformCount = 0ULL;
    ActorID RootFirstChildId      = InvalidActorID;
    ActorID RootLastChildId       = InvalidActorID;
    UInt64 PublicationSequence    = 0ULL;
    Bool bStructureDirty          = false;
    Bool bAnyWorldDirty           = false;
    Bool bAnyPublicationDirty     = false;
};

} // namespace Lumen

#include "Inline/SceneHierarchy.inl"
