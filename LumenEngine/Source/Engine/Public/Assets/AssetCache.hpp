/**
 * @file AssetCache.hpp
 * @brief Thread-safe asset caching for Lumen Engine.
 */

#pragma once

#include "Container/SharedPtr.hpp"
#include "Container/String.hpp"

#include "Cache/Cache.hpp"
#include "Cache/CachePolicy.hpp"

#include "Hash/Hash.hpp"

namespace LumenEngine
{

namespace Engine
{
    /**
     * @class TAssetCache
     * @brief A thread-safe cache for engine assets.
     * @tparam Type The type of asset to cache.
     */
    template <typename Type> class TAssetCache final : public FNonCopyable
    {
    public:

        TAssetCache () noexcept  = default;
        ~TAssetCache () noexcept = default;

    public:

        /** @brief Adds an asset to the cache. */
        void Add ( const FString &InKey, const TSharedPtr<Type> &InAsset ) noexcept;

        /** @brief Finds an asset in the cache. Returns nullptr if not found. */
        [[nodiscard]] TSharedPtr<Type> Find ( const FString &InKey ) const noexcept;

        /** @brief Removes an asset from the cache. */
        Bool Remove ( const FString &InKey ) noexcept;

        /** @brief Clears the entire cache. */
        void Clear () noexcept;

        /** @brief Returns the number of assets in the cache. */
        [[nodiscard]] USize Count () const noexcept;

    private:

        /** Internal cache using Core TCache with shared-read/exclusive-write mutex. */
        TCache<FString, TSharedPtr<Type>, Cache::FNoEviction<FString, TSharedPtr<Type>>, Hash::TStdHashAdapter<FString>> InternalCache;
    };

} // namespace Engine

} // namespace LumenEngine

#include "Inline/AssetCache.inl"