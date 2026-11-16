/**
 * @file RHIObjectPool.inl
 * @brief Implements the fixed-capacity object pool.
 */
#pragma once
#include <Memory/RHIObjectPool.hpp>
#include <memory>
#include <utility>

template <typename ObjectType>
Lumen::TRHIObjectPool<ObjectType>::TRHIObjectPool( const UInt32 InCapacity ) noexcept
    : Objects( MakeUnique<FSlot[]>( InCapacity ) ), Slots( InCapacity )
{
}

template <typename ObjectType>
template <typename... ArgumentTypes>
ObjectType *Lumen::TRHIObjectPool<ObjectType>::Create ( ArgumentTypes &&...InArguments ) noexcept
{
    const auto Allocation = Slots.Allocate( 1U );
    if ( not Allocation )
    {
        return nullptr;
    }
    FSlot &Slot = Objects[Allocation->Start];
    return std::construct_at( reinterpret_cast<ObjectType *>( Slot.Storage ), std::forward<ArgumentTypes>( InArguments )... );
}

template <typename ObjectType> void Lumen::TRHIObjectPool<ObjectType>::Destroy ( ObjectType *InObject ) noexcept
{
    if ( InObject == nullptr )
    {
        return;
    }
    std::destroy_at( InObject );
    const auto *Slot  = reinterpret_cast<const FSlot *>( InObject );
    const USize Index = static_cast<USize>( Slot - Objects.Get() );
    Slots.Free( FIndiceAllocation( static_cast<UInt32>( Index ), 1U ) );
}
