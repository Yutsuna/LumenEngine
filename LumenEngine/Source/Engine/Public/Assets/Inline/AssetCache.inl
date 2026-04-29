/**
 * @file AssetCache.inl
 * @brief Template implementation for the TAssetCache class.
 */

#pragma once

#include "Assets/AssetCache.hpp"

template <typename Type> void LumenEngine::Engine::TAssetCache<Type>::Add ( const FString &InKey, const TSharedPtr<Type> &InAsset ) noexcept
{
    InternalCache.Put( InKey, InAsset );
}

template <typename Type> LumenEngine::TSharedPtr<Type> LumenEngine::Engine::TAssetCache<Type>::Find ( const FString &InKey ) const noexcept
{
    TOptional<TSharedPtr<Type>> Result = InternalCache.TryGetCopy( InKey );
    LUMEN_OPTIONAL( Result );

    return Result.value();
}

template <typename Type> LumenEngine::Bool LumenEngine::Engine::TAssetCache<Type>::Remove ( const FString &InKey ) noexcept
{
    return InternalCache.Erase( InKey );
}

template <typename Type> void LumenEngine::Engine::TAssetCache<Type>::Clear () noexcept
{
    InternalCache.Clear();
}

template <typename Type> LumenEngine::USize LumenEngine::Engine::TAssetCache<Type>::Count () const noexcept
{
    return InternalCache.Size();
}