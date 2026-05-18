/**
 * @file ResourceRegistry.inl
 * @brief Inline implementation of TResourceRegistry.
 */

#pragma once

#include "RHI/RHI.hpp"
#include "RHI/ResourceRegistry.hpp"

#include "Logging/Logger.hpp"

#include <utility>

template <typename ResourceType, typename Tag> LumenEngine::UInt16 LumenEngine::RHI::TResourceRegistry<ResourceType, Tag>::AllocateSlot ()
{
    UInt16 Index = 0;

    if ( not FreeIndices.empty() )
    {
        Index = FreeIndices.back();
        FreeIndices.pop_back();
    }
    else
    {
        Index = static_cast<UInt16>( Slots.size() );
        Slots.emplace_back();
    }

    return Index;
}

template <typename ResourceType, typename Tag>
LumenEngine::RHI::TRenderResourceHandle<Tag> LumenEngine::RHI::TResourceRegistry<ResourceType, Tag>::Insert ( ResourceType &&InResource )
{
    const UInt16 Index = AllocateSlot();
    FSlot &Slot        = Slots[Index];

    Slot.Data      = std::move( InResource );
    Slot.bIsActive = true;

    return HandleType( Index, Slot.Generation );
}

template <typename ResourceType, typename Tag>
LumenEngine::RHI::TRenderResourceHandle<Tag> LumenEngine::RHI::TResourceRegistry<ResourceType, Tag>::Insert ( const ResourceType &InResource )
{
    const UInt16 Index = AllocateSlot();
    FSlot &Slot        = Slots[Index];

    Slot.Data      = InResource;
    Slot.bIsActive = true;

    return HandleType( Index, Slot.Generation );
}

template <typename ResourceType, typename Tag> void LumenEngine::RHI::TResourceRegistry<ResourceType, Tag>::Remove ( HandleType InHandle )
{
    if ( not IsValid( InHandle ) )
    {
        LUMEN_LOG_ERROR( LogRHI, "Attempted to remove invalid handle (Index: {}, Generation: {})", InHandle.GetIndex(), InHandle.GetGeneration() );
        return;
    }

    const UInt16 Index = InHandle.GetIndex();
    FSlot &Slot        = Slots[Index];

    Slot.bIsActive = false;
    ++Slot.Generation;

    FreeIndices.push_back( Index );
}

template <typename ResourceType, typename Tag> LumenEngine::Bool LumenEngine::RHI::TResourceRegistry<ResourceType, Tag>::IsValid ( HandleType InHandle ) const noexcept
{
    if ( not InHandle.IsValid() )
    {
        return false;
    }

    const UInt16 Index      = InHandle.GetIndex();
    const UInt16 Generation = InHandle.GetGeneration();

    if ( Index >= Slots.size() )
    {
#if !defined( NDEBUG )
        LUMEN_LOG_ERROR( LogRHI, "Handle validation failed: Index {} out of bounds (Size: {})", Index, Slots.size() );
#endif
        return false;
    }

    const FSlot &Slot = Slots[Index];
    if ( not Slot.bIsActive )
    {
#if !defined( NDEBUG )
        LUMEN_LOG_ERROR( LogRHI, "Handle validation failed: Slot at Index {} is inactive", Index );
#endif
        return false;
    }

    if ( Slot.Generation != Generation )
    {
#if !defined( NDEBUG )
        LUMEN_LOG_ERROR( LogRHI, "Handle validation failed: Generation mismatch at Index {} (Handle: {}, Registry: {})", Index, Generation, Slot.Generation );
#endif
        return false;
    }

    return true;
}

template <typename ResourceType, typename Tag> ResourceType *LumenEngine::RHI::TResourceRegistry<ResourceType, Tag>::Get ( HandleType InHandle )
{
    if ( not IsValid( InHandle ) )
    {
        return nullptr;
    }
    return &Slots[InHandle.GetIndex()].Data;
}

template <typename ResourceType, typename Tag> const ResourceType *LumenEngine::RHI::TResourceRegistry<ResourceType, Tag>::Get ( HandleType InHandle ) const
{
    if ( not IsValid( InHandle ) )
    {
        return nullptr;
    }
    return &Slots[InHandle.GetIndex()].Data;
}

template <typename ResourceType, typename Tag> void LumenEngine::RHI::TResourceRegistry<ResourceType, Tag>::Clear ()
{
    Slots.clear();
    FreeIndices.clear();
}

template <typename ResourceType, typename Tag> template <typename Func> void LumenEngine::RHI::TResourceRegistry<ResourceType, Tag>::ForEach ( Func InFunc )
{
    for ( FSlot &Slot : Slots )
    {
        if ( Slot.bIsActive )
        {
            InFunc( Slot.Data );
        }
    }
}
