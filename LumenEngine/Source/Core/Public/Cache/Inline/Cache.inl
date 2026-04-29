/**
 * @file Cache.inl
 * @brief Inline implementations for TCache methods.
 */

#pragma once

#include "Cache/Cache.hpp"
#include "HAL/Mutex.hpp"

#define LUMEN_CACHE_TEMPLATE                                                                                                                                             \
    template <LumenEngine::Concepts::CCacheKey KeyType,            /**/                                                                                                  \
              LumenEngine::Concepts::CMoveConstructible ValueType, /**/                                                                                                  \
              typename PolicyType,                                 /**/                                                                                                  \
              typename HashFunc>                                   /**/                                                                                                  \
        requires LumenEngine::Concepts::CEvictionPolicy<PolicyType, KeyType, ValueType>

#define LUMEN_CACHE_CLASS LumenEngine::TCache<KeyType, ValueType, PolicyType, HashFunc>

/**
 * Ctor
 */

LUMEN_CACHE_TEMPLATE
LUMEN_CACHE_CLASS::TCache( const USize InMaxSize ) noexcept : MaxSize( InMaxSize )
{
    /* Ctor */
}

/**
 * Public Methods
 */

LUMEN_CACHE_TEMPLATE
const ValueType *LUMEN_CACHE_CLASS::TryGet ( const KeyType &InKey ) noexcept
{
    TSharedLock<FSharedMutex> ReadLock( Mutex );
    auto It = Storage.find( InKey );

    if ( It == Storage.end() )
    {
        Counters.RecordMiss();
        return nullptr;
    }

    Counters.RecordHit();
    EvictionPolicy.OnAccess( InKey );
    return &It->second;
}

LUMEN_CACHE_TEMPLATE
LumenEngine::TOptional<ValueType> LUMEN_CACHE_CLASS::TryGetCopy ( const KeyType &InKey ) noexcept
{
    TSharedLock<FSharedMutex> ReadLock( Mutex );
    auto It = Storage.find( InKey );

    if ( It == Storage.end() )
    {
        Counters.RecordMiss();
        return {};
    }

    Counters.RecordHit();
    EvictionPolicy.OnAccess( InKey );
    return It->second;
}

LUMEN_CACHE_TEMPLATE
LumenEngine::Bool LUMEN_CACHE_CLASS::Contains ( const KeyType &InKey ) const noexcept
{
    TSharedLock<FSharedMutex> ReadLock( Mutex );

    return Storage.contains( InKey );
}

LUMEN_CACHE_TEMPLATE
void LUMEN_CACHE_CLASS::Put ( const KeyType &InKey, const ValueType &InValue )
{
    TUniqueLock<FSharedMutex> WriteLock( Mutex );
    PutImplementation( InKey, InValue );
}

LUMEN_CACHE_TEMPLATE
void LUMEN_CACHE_CLASS::Put ( const KeyType &InKey, ValueType &&InValue )
{
    TUniqueLock<FSharedMutex> WriteLock( Mutex );
    PutImplementation( InKey, std::move( InValue ) );
}

LUMEN_CACHE_TEMPLATE
LumenEngine::Bool LUMEN_CACHE_CLASS::Erase ( const KeyType &InKey )
{
    TUniqueLock<FSharedMutex> WriteLock( Mutex );
    auto It = Storage.find( InKey );

    if ( It == Storage.end() )
    {
        return false;
    }

    EvictionPolicy.OnErase( InKey );
    Storage.erase( It );
    return true;
}

LUMEN_CACHE_TEMPLATE
LumenEngine::USize LUMEN_CACHE_CLASS::Clear () noexcept
{
    TUniqueLock<FSharedMutex> WriteLock( Mutex );
    const USize Removed = Storage.size();

    Storage.clear();
    EvictionPolicy.OnClear();
    return Removed;
}

LUMEN_CACHE_TEMPLATE
LumenEngine::USize LUMEN_CACHE_CLASS::Size () const noexcept
{
    TSharedLock<FSharedMutex> ReadLock( Mutex );
    return Storage.size();
}

LUMEN_CACHE_TEMPLATE
LumenEngine::Bool LUMEN_CACHE_CLASS::IsEmpty () const noexcept
{
    TSharedLock<FSharedMutex> ReadLock( Mutex );
    return Storage.IsEmpty();
}

LUMEN_CACHE_TEMPLATE
constexpr LumenEngine::USize LUMEN_CACHE_CLASS::GetMaxSize () const noexcept
{
    return MaxSize;
}

LUMEN_CACHE_TEMPLATE
void LUMEN_CACHE_CLASS::SetMaxSize ( const USize InNewMaxSize ) noexcept
{
    TUniqueLock<FSharedMutex> WriteLock( Mutex );

    MaxSize = InNewMaxSize;
    EvictToCapacity();
}

LUMEN_CACHE_TEMPLATE
LumenEngine::Cache::FCacheStats LUMEN_CACHE_CLASS::GetStats () const noexcept
{
    return Counters.Snapshot();
}

LUMEN_CACHE_TEMPLATE
void LUMEN_CACHE_CLASS::ResetStats () noexcept
{
    Counters.Reset();
}

/**
 * Private Methods
 */

LUMEN_CACHE_TEMPLATE
template <typename ForwardValue> void LUMEN_CACHE_CLASS::PutImplementation ( const KeyType &InKey, ForwardValue &&InValue )
{
    auto It = Storage.find( InKey );

    if ( It != Storage.end() )
    {
        It->second = std::forward<ForwardValue>( InValue );
        EvictionPolicy.OnAccess( InKey );
        return;
    }

    if ( Storage.size() >= MaxSize )
    {
        EvictOne();
    }

    Storage.emplace( InKey, std::forward<ForwardValue>( InValue ) );
    EvictionPolicy.OnInsert( InKey, InValue );
    Counters.RecordInsert();
}

LUMEN_CACHE_TEMPLATE
void LUMEN_CACHE_CLASS::EvictOne ()
{
    const KeyType &Victim = EvictionPolicy.Victim();
    auto It               = Storage.find( Victim );

    if ( It != Storage.end() )
    {
        Storage.erase( It );
        EvictionPolicy.OnErase( Victim );
        Counters.RecordEviction();
    }
}

LUMEN_CACHE_TEMPLATE
void LUMEN_CACHE_CLASS::EvictToCapacity ()
{
    while ( Storage.size() > MaxSize )
    {
        EvictOne();
    }
}

#undef LUMEN_CACHE_TEMPLATE
#undef LUMEN_CACHE_CLASS