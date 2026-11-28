/**
 * @file CameraRegistry.hpp
 * @brief Bounded, actor-id-indexed storage for generic camera components.
 */
#pragma once

#include <SceneExport.hpp>

#include <Actor/ActorTypes.hpp>
#include <Camera/CameraTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <HAL/PlatformMutex.hpp>
#include <Rendering/RenderRecords.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Map.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

/**
 * @class FCameraComponentRegistry
 * @brief Pre-reserved camera component column consumed by world render extraction.
 * @details Every mutation is bounded and allocation-free after construction. The registry is the single source of
 *          truth for camera records; actors reach it through injection, as they do with FSpatialRegistry.
 */
class SCENE_EXPORT FCameraComponentRegistry final : public FNonCopyable
{
public:

    /** @brief Creates an empty registry whose flat storage is reserved for InCapacity cameras. */
    explicit FCameraComponentRegistry ( USize InCapacity ) noexcept;
    ~FCameraComponentRegistry () noexcept = default;

public:

    /** @brief Registers one camera or returns AlreadyRegistered / CapacityExceeded without partial insertion. */
    [[nodiscard]] TExpected<void, ECameraComponentError> TryRegister ( ActorID InId,
                                                                       const FCameraComponentDesc &InDesc ) noexcept;

    /** @brief Removes one camera; UnknownActor is reported but is not a fatal condition. */
    [[nodiscard]] TExpected<void, ECameraComponentError> TryUnregister ( ActorID InId ) noexcept;

    /** @brief Replaces the world-to-camera view matrix of one camera. */
    [[nodiscard]] TExpected<void, ECameraComponentError> TrySetView ( ActorID InId, const Maths::FMat4x4f &InView ) noexcept;

    /** @brief Replaces the camera-to-clip projection matrix of one camera. */
    [[nodiscard]] TExpected<void, ECameraComponentError> TrySetProjection ( ActorID InId,
                                                                            const Maths::FMat4x4f &InProjection ) noexcept;

    /** @brief Replaces clipping planes and viewport of one camera. */
    [[nodiscard]] TExpected<void, ECameraComponentError>
    TrySetClipping ( ActorID InId, Float32 InNearPlane, Float32 InFarPlane, const FRenderViewport &InViewport ) noexcept;

    /** @brief Replaces the selection flags of one camera. */
    [[nodiscard]] TExpected<void, ECameraComponentError> TrySetFlags ( ActorID InId, ERenderCameraFlags InFlags ) noexcept;

    /**
     * @brief Appends one render record per registered camera to a pre-reserved output vector.
     * @details The output must have room for GetCount() additional records; no allocation happens here.
     */
    void CollectRecords ( TVector<FRenderCameraRecord> &OutRecords ) const noexcept;

    /** @return Number of cameras this registry can hold. */
    [[nodiscard]] USize GetCapacity () const noexcept;

    /** @return Number of cameras currently registered. */
    [[nodiscard]] USize GetCount () const noexcept;

private:

    struct FCameraEntry
    {
        ActorID Id = InvalidActorID;
        FCameraComponent Component;
    };

    /** @brief Returns the component of InId, or an empty optional when no camera is registered for it. */
    [[nodiscard]] TOptional<FCameraComponent *> FindComponent ( ActorID InId ) noexcept;

private:

    mutable HAL::FPlatformSharedMutex RegistryMutex;
    USize Capacity = 0ULL;
    TVector<FCameraEntry> Entries;
    TMap<ActorID, USize> IdToIndex;
};

} // namespace Lumen
