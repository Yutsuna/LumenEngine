/**
 * @file AssetManager.inl
 * @brief Inline implementations for the FAssetManager class.
 */

#pragma once

#include "Assets/AssetManager.hpp"

inline LumenEngine::Engine::TAssetCache<LumenEngine::Renderer::FRenderMesh> &LumenEngine::Engine::FAssetManager::GetMeshCache () noexcept
{
    return MeshCache;
}

inline LumenEngine::Engine::TAssetCache<LumenEngine::Renderer::FRenderShader> &LumenEngine::Engine::FAssetManager::GetShaderCache () noexcept
{
    return ShaderCache;
}
