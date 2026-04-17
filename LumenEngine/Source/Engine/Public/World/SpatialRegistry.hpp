/**
 * @file SpatialRegistry.hpp
 * @brief Data-Oriented registry for spatial and renderable data.
 */

#pragma once

#include "Actor/ActorTypes.hpp"
#include "CoreTypes.hpp"

#include "Container/Map.hpp"
#include "Container/Vector.hpp"

#include "HAL/SharedMutex.hpp"
#include "Maths/Matrix.hpp"
#include "RHI/RHITypes.hpp"

#include "Container/AtomicBitset.hpp"
#include "Thread/TripleBuffer.hpp"

namespace LumenEngine
{

namespace Engine
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

    /**
     * @struct FSpatialRegistryData
     * @brief SoA storage for actor spatial state.
     */
    struct LUMEN_ENGINE_API FSpatialRegistryData
    {
        TVector<ActorID> EntityIDs;
        TVector<Maths::FMatrix4x4f> Transforms;
        TVector<RHI::FMeshHandle> Meshes;
        TVector<RHI::FPipelineHandle> Shaders;
        TMap<ActorID, USize> IDToIndex;

        using FMapIterator = TMap<ActorID, USize>::const_iterator;
    };

    /**
     * @class FSpatialRegistry
     * @brief High-performance bridge between Actors and the Renderer.
     */
    class LUMEN_ENGINE_API FSpatialRegistry final : public FNonCopyable
    {
    public:

        /** @brief Gets the singleton instance of the spatial registry. */
        static FSpatialRegistry &Get () noexcept;

    public:

        /** @brief Registers a new spatial entity in the registry. */
        void RegisterSpatialEntity ( ActorID InId );

        /** @brief Unregisters a spatial entity from the registry. */
        void UnregisterSpatialEntity ( ActorID InId );

        /**
         * @brief Updates the transform of a spatial entity. (Thread-safe)
         * @param InId The ID of the entity to update.
         * @param InTransform The new transform.
         */
        void UpdateTransform ( ActorID InId, const Maths::FMatrix4x4f &InTransform ) noexcept;

        /** @brief Assigns render data to a spatial entity. */
        void AssignRenderData ( ActorID InId, RHI::FMeshHandle InMesh, RHI::FPipelineHandle InShader ) noexcept;

        /** @brief Publishes the current state of the registry. */
        void Publish () noexcept;

        /** @brief Swaps the read buffers. */
        void SwapReadBuffers () noexcept;

        /** @brief Gets a read-only snapshot of the current registry state. */
        [[nodiscard]] const FSpatialRegistryData &GetReadSnapshot () const noexcept;

    private:

        FSpatialRegistry () noexcept = default;

        /** Internal helper to mark a component dirty across all TripleBuffer slots */
        void MarkComponentDirty ( USize InIndex, ESpatialComponentType InComponent ) noexcept;

    private:

        mutable FSharedMutex RegistryMutex;
        FSpatialRegistryData WorkingData;
        Parallel::TTripleBuffer<FSpatialRegistryData> SnapshotBuffer;

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

        FDeltaTracker DeltaTrackers[3];
    };

} // namespace Engine

} // namespace LumenEngine