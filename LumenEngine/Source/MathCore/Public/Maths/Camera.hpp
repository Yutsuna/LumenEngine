/**
 * @file Camera.hpp
 * @brief 3D and 2D compliant camera abstraction for LumenEngine.
 */

#pragma once

#include "CoreTypes.hpp"

#include "Definitions.hpp"
#include "EnumFlags.hpp"
#include "Maths/Matrix.hpp"
#include "Maths/Vec.hpp"

namespace LumenEngine
{

namespace Maths
{

    /**
     * @enum ECameraProjectionMode
     * @brief Defines how the camera projects the 3D scene onto the 2D screen.
     */
    enum class ECameraProjectionMode : UInt8
    {
        Perspective, /**< Standard 3D projection with depth foreshortening. */
        Orthographic /**< Parallel projection for 2D, UI, or technical 3D views. */
    };
    LUMEN_ENUM_FLAGS( ECameraProjectionMode )

    enum class ECameraDirtyFlags : UInt8
    {
        None           = 0,                                 /**< No changes, matrices are up to date. */
        View           = 1 << 0,                            /**< View parameters changed, view matrix needs update. */
        Projection     = 1 << 1,                            /**< Projection parameters changed, projection matrix needs update. */
        ViewProjection = 1 << 2,                            /**< Both view and projection changed, combined matrix needs update. */
        All            = View | Projection | ViewProjection /**< All matrices need update. */
    };
    LUMEN_ENUM_FLAGS( ECameraDirtyFlags )

    /**
     * @class FCamera
     * @brief A unified camera class handling both 3D perspective and 2D orthographic projections.
     */
    class alignas( 64 ) LUMEN_ENGINE_API FCamera final
    {
    public:

        constexpr FCamera () noexcept = default;

    public:

        /** @brief Sets the perspective projection parameters. */
        void SetPerspective ( const Float32 InFovDegrees, const Float32 InAspectRatio, const Float32 InNear, const Float32 InFar ) noexcept;

        /** @brief Sets the camera's view using LookAt logic. */
        void LookAt ( const FVec3f &InEye, const FVec3f &InTarget, const FVec3f &InUp = { 0.0F, -1.0F, 0.0F } ) noexcept;

        /** @brief Updates the internal matrices if dirty. */
        void Update () noexcept;

    public:

        /** @brief Returns the view matrix. */
        [[nodiscard]] const FMatrix4x4f &GetViewMatrix () const noexcept;

        /** @brief Returns the projection matrix. */
        [[nodiscard]] const FMatrix4x4f &GetProjectionMatrix () const noexcept;

        /** @brief Combined View-Projection matrix. */
        [[nodiscard]] FMatrix4x4f GetViewProjectionMatrix () const noexcept;

    private:

        FVec3f Position{ 0.0F, 0.0F, -3.0F };
        FVec3f Target{ 0.0F, 0.0F, 0.0F };
        FVec3f UpVector{ 0.0F, -1.0F, 0.0F };

        Float32 FieldOfView{ 60.0F }, AspectRatio{ 1.77F }, NearPlane{ 0.1F }, FarPlane{ 100.0F };

        FMatrix4x4f ViewMatrix           = FMatrix4x4f::Identity();
        FMatrix4x4f ProjectionMatrix     = FMatrix4x4f::Identity();
        FMatrix4x4f ViewProjectionMatrix = FMatrix4x4f::Identity();

        ECameraDirtyFlags DirtyFlags = ECameraDirtyFlags::All;
    };

} // namespace Maths

} // namespace LumenEngine