/**
 * @file RHIObjectCache.inl
 * @brief Implements the synchronized object cache template.
 */
#pragma once

#include <Caches/RHIObjectCache.hpp>
#include <HAL/LockGuard.hpp>

template <typename KeyType, typename ValueType, typename HashType>
Lumen::TRHIObjectCache<KeyType, ValueType, HashType>::TRHIObjectCache( const USize InMaxEntries,
                                                                       const ERHICacheEvictionPolicy InPolicy ) noexcept
    : MaxEntries( InMaxEntries == 0U ? 1U : InMaxEntries ), Policy( InPolicy )
{
}

template <typename KeyType, typename ValueType, typename HashType>
Lumen::TSharedPtr<ValueType> Lumen::TRHIObjectCache<KeyType, ValueType, HashType>::Find ( const KeyType &InKey ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const auto Found = Entries.find( InKey );
    if ( Found == Entries.end() )
    {
        Stats.OnLookup( false );
        return {};
    }
    ++Sequence;
    Found->second.LastUsed = Sequence;
    Stats.OnLookup( true );
    return Found->second.Value;
}

template <typename KeyType, typename ValueType, typename HashType>
Lumen::TExpected<Lumen::TSharedPtr<ValueType>, Lumen::FRHIError>
Lumen::TRHIObjectCache<KeyType, ValueType, HashType>::GetOrCreate (
    const KeyType &InKey, TFunction<TExpected<TSharedRef<ValueType>, FRHIError>()> InFactory, const UInt64 InBytes ) noexcept
{
    if ( TSharedPtr<ValueType> Existing = Find( InKey ) )
    {
        return Existing;
    }
    auto Created = InFactory();
    if ( not Created )
    {
        return TUnexpected<FRHIError>( Created.Error() );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const auto Existing = Entries.find( InKey );
    if ( Existing != Entries.end() )
    {
        return Existing->second.Value;
    }
    if ( not MakeRoom() )
    {
        return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Cache, "Object cache capacity is exhausted" ) );
    }
    ++Sequence;
    FEntry Entry{ .Value = *Created, .Bytes = InBytes, .LastUsed = Sequence };
    Entries.emplace( InKey, std::move( Entry ) );
    Stats.OnInsert( InBytes );
    return TSharedPtr<ValueType>( *Created );
}

template <typename KeyType, typename ValueType, typename HashType>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::TRHIObjectCache<KeyType, ValueType, HashType>::Store (
    const KeyType &InKey, TSharedRef<ValueType> InValue, const UInt64 InBytes ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const auto Existing = Entries.find( InKey );
    if ( Existing != Entries.end() )
    {
        Stats.OnRemove( Existing->second.Bytes );
        Entries.erase( Existing );
    }
    if ( not MakeRoom() )
    {
        return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Cache, "Object cache capacity is exhausted" ) );
    }
    ++Sequence;
    FEntry Entry{ .Value = InValue, .Bytes = InBytes, .LastUsed = Sequence };
    Entries.emplace( InKey, std::move( Entry ) );
    Stats.OnInsert( InBytes );
    return {};
}

template <typename KeyType, typename ValueType, typename HashType>
void Lumen::TRHIObjectCache<KeyType, ValueType, HashType>::Remove ( const KeyType &InKey ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const auto Found = Entries.find( InKey );
    if ( Found != Entries.end() )
    {
        Stats.OnRemove( Found->second.Bytes );
        Entries.erase( Found );
    }
}

template <typename KeyType, typename ValueType, typename HashType>
void Lumen::TRHIObjectCache<KeyType, ValueType, HashType>::Clear () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Entries.clear();
    Stats.Reset();
}

template <typename KeyType, typename ValueType, typename HashType>
Lumen::FRHICacheStats Lumen::TRHIObjectCache<KeyType, ValueType, HashType>::GetStats () const noexcept
{
    return Stats.Snapshot();
}

template <typename KeyType, typename ValueType, typename HashType>
Lumen::USize Lumen::TRHIObjectCache<KeyType, ValueType, HashType>::GetEntryCount () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Entries.size();
}

template <typename KeyType, typename ValueType, typename HashType>
Lumen::Bool Lumen::TRHIObjectCache<KeyType, ValueType, HashType>::MakeRoom () noexcept
{
    while ( Entries.size() >= MaxEntries )
    {
        if ( Policy != ERHICacheEvictionPolicy::LeastRecentlyUsed or Entries.empty() )
        {
            return false;
        }
        EvictOne();
    }
    return true;
}

template <typename KeyType, typename ValueType, typename HashType>
void Lumen::TRHIObjectCache<KeyType, ValueType, HashType>::EvictOne () noexcept
{
    auto Oldest = Entries.begin();
    for ( auto Iterator = Entries.begin(); Iterator != Entries.end(); ++Iterator )
    {
        if ( Iterator->second.LastUsed < Oldest->second.LastUsed )
        {
            Oldest = Iterator;
        }
    }
    if ( Oldest != Entries.end() )
    {
        Stats.OnEvict( Oldest->second.Bytes );
        Entries.erase( Oldest );
    }
}
