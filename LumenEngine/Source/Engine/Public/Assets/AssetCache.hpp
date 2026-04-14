/**
 * @file AssetCache.hpp
 * @brief Thread-safe asset caching for Lumen Engine.
 */

#pragma once

#include "Container/Map.hpp"
#include "Container/SharedPtr.hpp"
#include "Container/String.hpp"

#include "HAL/Mutex.hpp"

namespace LumenEngine
{

namespace Engine
{

    /**
     * @class TAssetCache
     * @brief A thread-safe cache for engine assets.
     * @tparam Type The type of asset to cache.
     */
    template <typename Type> class TAssetCache final
    {
    public:

        TAssetCache () noexcept  = default;
        ~TAssetCache () noexcept = default;

    public:

        /**
         * @brief Adds an asset to the cache.
         * @param InKey The unique identifier for the asset.
         * @param InAsset The asset to add.
         */
        void Add ( const FString &InKey, const TSharedPtr<Type> &InAsset ) noexcept;

        /**
         * @brief Finds an asset in the cache.
         * @param InKey The unique identifier for the asset.
         * @return A TSharedPtr to the asset if found, or nullptr otherwise.
         */
        [[nodiscard]] TSharedPtr<Type> Find ( const FString &InKey ) const noexcept;

        /**
         * @brief Removes an asset from the cache.
         * @param InKey The unique identifier for the asset.
         * @return True if the asset was removed, false if not found.
         */
        bool Remove ( const FString &InKey ) noexcept;

        /** @brief Clears the entire cache. */
        void Clear () noexcept;

        /** @brief Returns the number of assets in the cache. */
        [[nodiscard]] USize Count () const noexcept;

    private:

        /** Internal storage for assets */
        mutable FMutex CacheMutex;
        TMap<FString, TSharedPtr<Type>> CachedAssets;
    };

} // namespace Engine

} // namespace LumenEngine

#include "Inline/AssetCache.inl"
