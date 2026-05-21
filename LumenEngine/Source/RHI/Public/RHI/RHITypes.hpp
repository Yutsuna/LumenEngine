/**
 * @file RHITypes.hpp
 * @brief Common types, strongly typed handles, and global uniforms for the rendering system.
 */

#pragma once

#include "Container/Vector.hpp"
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
     * @brief Strongly typed handle storing a Generational Index (16-bit Index, 16-bit Generation).
     */
    template <typename Tag> struct TRenderResourceHandle
    {
        static constexpr UInt32 InvalidID = 0xFFFFFFFF;

        UInt32 ID = InvalidID;

        constexpr TRenderResourceHandle () noexcept = default;
        constexpr explicit TRenderResourceHandle ( UInt32 InID ) noexcept;
        constexpr explicit TRenderResourceHandle ( UInt16 InIndex, UInt16 InGeneration ) noexcept;

        [[nodiscard]] constexpr UInt16 GetIndex () const noexcept;
        [[nodiscard]] constexpr UInt16 GetGeneration () const noexcept;
        [[nodiscard]] constexpr Bool IsValid () const noexcept;

        constexpr Bool operator==( const TRenderResourceHandle &Other ) const noexcept;
        constexpr Bool operator!=( const TRenderResourceHandle &Other ) const noexcept;
    };

    using FMeshHandle     = TRenderResourceHandle<FMeshTag>;
    using FPipelineHandle = TRenderResourceHandle<FPipelineTag>;
    using FTextureHandle  = TRenderResourceHandle<FTextureTag>;

    /** Global limit for GPU instances in the scene */
    static constexpr USize MaxInstances = 65536ULL;

    /**
     * @brief Raw shader bytecode container (SPIR-V)
     */
    using FShaderByteCode = TVector<UInt32>;

    /**
     * @struct FGraphicsPipelineDesc
     * @brief Describes the state and shaders for a graphics pipeline.
     */
    struct FGraphicsPipelineDesc
    {
        FShaderByteCode VertexShader;
        FShaderByteCode FragmentShader;
    };

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

    /**
     * @struct FSceneSnapshot
     * @brief Render-thread snapshot streamed from the game-thread spatial registry.
     */
    struct FSceneSnapshot
    {
        TVector<Maths::FMatrix4x4f> Transforms;
        TVector<FMeshHandle> Meshes;
        TVector<FPipelineHandle> Shaders;
    };

} // namespace RHI

} // namespace LumenEngine

#include "Inline/RHITypes.inl"
