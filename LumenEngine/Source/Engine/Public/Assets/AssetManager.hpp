/**
 * @file AssetManager.hpp
 * @brief Centralized manager for engine assets in Lumen Engine.
 */

#pragma once

#include "Definitions.hpp"
#include "NonCopyable.hpp"

#include "AssetCache.hpp"

#include "Graphics/Resources/RenderMaterial.hpp"
#include "Graphics/Resources/RenderMesh.hpp"
#include "Graphics/Resources/RenderShader.hpp"

namespace LumenEngine
{

namespace Engine
{

    /**
     * @class FAssetManager
     * @brief A centralized manager for engine assets.
     */
    class LUMEN_ENGINE_API FAssetManager final : public FNonCopyable
    {
    public:

        /** @return The global asset manager instance. */
        static FAssetManager &Get () noexcept;

    public:

        FAssetManager ()  = default;
        ~FAssetManager () = default;

    public:

        /** @return The mesh asset cache. */
        [[nodiscard]] TAssetCache<Renderer::FRenderMesh> &GetMeshCache () noexcept;

        /** @return The shader asset cache. */
        [[nodiscard]] TAssetCache<Renderer::FRenderShader> &GetShaderCache () noexcept;

        /** @return The material asset cache. */
        [[nodiscard]] TAssetCache<Renderer::FRenderMaterial> &GetMaterialCache () noexcept;

    private:

        /** The mesh asset cache. */
        TAssetCache<Renderer::FRenderMesh> MeshCache;

        /** The shader asset cache. */
        TAssetCache<Renderer::FRenderShader> ShaderCache;

        /** The material asset cache. */
        TAssetCache<Renderer::FRenderMaterial> MaterialCache;
    };

} // namespace Engine

} // namespace LumenEngine

#include "Inline/AssetManager.inl"
