/**
 * @file AssetManager.cpp
 * @brief Implementation of the FAssetManager class.
 */

#include "Assets/AssetManager.hpp"

LumenEngine::Engine::FAssetManager &LumenEngine::Engine::FAssetManager::Get () noexcept
{
    static FAssetManager Instance;
    return Instance;
}
