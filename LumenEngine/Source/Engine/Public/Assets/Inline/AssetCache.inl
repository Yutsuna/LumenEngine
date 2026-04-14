/**
 * @file AssetCache.inl
 * @brief Template implementation for the TAssetCache class.
 */

#pragma once

#include "Assets/AssetCache.hpp"

template <typename Type> void LumenEngine::Engine::TAssetCache<Type>::Add ( const FString &InKey, const TSharedPtr<Type> &InAsset ) noexcept
{
    TLockGuard<FMutex> Lock( CacheMutex );
    CachedAssets[InKey] = InAsset;
}

template <typename Type> LumenEngine::TSharedPtr<Type> LumenEngine::Engine::TAssetCache<Type>::Find ( const FString &InKey ) const noexcept
{
    TLockGuard<FMutex> Lock( CacheMutex );
    using TMapIterator    = typename TMap<FString, TSharedPtr<Type>>::const_iterator;
    const TMapIterator It = CachedAssets.find( InKey );

    if ( It != CachedAssets.end() )
    {
        return It->second;
    }
    return nullptr;
}

template <typename Type> bool LumenEngine::Engine::TAssetCache<Type>::Remove ( const FString &InKey ) noexcept
{
    TLockGuard<FMutex> Lock( CacheMutex );
    return CachedAssets.erase( InKey ) > 0;
}

template <typename Type> void LumenEngine::Engine::TAssetCache<Type>::Clear () noexcept
{
    TLockGuard<FMutex> Lock( CacheMutex );
    CachedAssets.clear();
}

template <typename Type> LumenEngine::USize LumenEngine::Engine::TAssetCache<Type>::Count () const noexcept
{
    TLockGuard<FMutex> Lock( CacheMutex );
    return static_cast<USize>( CachedAssets.size() );
}
