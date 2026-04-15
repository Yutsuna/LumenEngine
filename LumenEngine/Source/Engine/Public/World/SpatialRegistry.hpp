/**
 * @file SpatialRegistry.hpp
 * @brief Data-Oriented registry for spatial and renderable data.
 */

#pragma once

#include "Actor/ActorTypes.hpp"
#include "CoreTypes.hpp"

#include "Container/Map.hpp"
#include "Container/Vector.hpp"

#include "HAL/Mutex.hpp"
#include "Maths/Matrix.hpp"
#include "RHI/RHITypes.hpp"

#include "Thread/TripleBuffer.hpp"

namespace LumenEngine
{

namespace Engine
{

    struct LUMEN_ENGINE_API FSpatialRegistryData
    {
        TVector<ActorID> EntityIDs;
        TVector<Maths::FMatrix4x4f> Transforms;
        TVector<RHI::FMeshHandle> Meshes;
        TVector<RHI::FPipelineHandle> Shaders;
        TMap<ActorID, USize> IDToIndex;
    };

    /**
     * @class FSpatialRegistry
     * @brief Maps an ActorID to spatial and render components using triple-buffered snapshots.
     */
    class LUMEN_ENGINE_API FSpatialRegistry final : public FNonCopyable
    {
    public:

        static FSpatialRegistry &Get () noexcept;

    public:

        void RegisterSpatialEntity ( ActorID InId );
        void UnregisterSpatialEntity ( ActorID InId );

        void UpdateTransform ( ActorID InId, const Maths::FMatrix4x4f &InTransform ) noexcept;
        void AssignRenderData ( ActorID InId, RHI::FMeshHandle InMesh, RHI::FPipelineHandle InShader ) noexcept;

        /** @brief Safely publishes the working state to the TripleBuffer lock-free snapshot. */
        void Publish () noexcept;

        /** @brief Promotes the latest published snapshot to be available for reading. */
        void SwapReadBuffers () noexcept;

        /** @brief Exposes the current read-only snapshot. Call SwapReadBuffers first. */
        [[nodiscard]] const FSpatialRegistryData &GetReadSnapshot () const noexcept;

    private:

        FSpatialRegistry () noexcept = default;

        mutable FMutex RegistryMutex;

        /** Working Data for writes during the active frame */
        FSpatialRegistryData WorkingData;

        /** Triple buffer for lock-free read access by the Renderer/SceneActor */
        Parallel::TTripleBuffer<FSpatialRegistryData> SnapshotBuffer;
    };

} // namespace Engine

} // namespace LumenEngine
