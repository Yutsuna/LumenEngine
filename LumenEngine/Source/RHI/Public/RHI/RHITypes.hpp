/**
 * @file RHITypes.hpp
 * @brief Common types, strongly typed handles, and global uniforms for the rendering system.
 */

#pragma once

#include "CoreTypes.hpp"

#include "Maths/Matrix.hpp"

namespace LumenEngine
{

namespace RHI
{

    /**
     * @tag FMeshTag
     * @brief Tag type for mesh resource handles.
     */
    struct FMeshTag
    {
    };

    /**
     * @tag FPipelineTag
     * @brief Tag type for pipeline resource handles.
     */
    struct FPipelineTag
    {
    };

    /**
     * @tag FTextureTag
     * @brief Tag type for texture resource handles.
     */
    struct FTextureTag
    {
    };

    /**
     * @struct TRenderResourceHandle
     * @brief Strongly typed handle to prevent mixing up resource IDs.
     */
    template <typename Tag> struct TRenderResourceHandle
    {
        static constexpr UInt32 InvalidID = 0xFFFFFFFF;
        UInt32 ID                         = 0xFFFFFFFF;

        constexpr TRenderResourceHandle () noexcept = default;
        constexpr explicit TRenderResourceHandle ( UInt32 InID ) noexcept;

        [[nodiscard]] constexpr Bool IsValid () const noexcept;

        constexpr Bool operator==( const TRenderResourceHandle &Other ) const noexcept;
        constexpr Bool operator!=( const TRenderResourceHandle &Other ) const noexcept;
    };

    using FMeshHandle     = TRenderResourceHandle<FMeshTag>;
    using FPipelineHandle = TRenderResourceHandle<FPipelineTag>;
    using FTextureHandle  = TRenderResourceHandle<FTextureTag>;

    /**
     * @struct FGlobalUniformData
     * @brief Global data sent from Game Thread to Render Thread via Triple Buffer.
     */
    struct FGlobalUniformData
    {
        Maths::FMatrix4x4f ViewProjectionMatrix = Maths::FMatrix4x4f::Identity();
        Float32 TimeSeconds                     = 0.0F;
        Float32 DeltaTime                       = 0.0F;
    };

} // namespace RHI

} // namespace LumenEngine

#include "Inline/RHITypes.inl"
