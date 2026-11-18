/**
 * @file RHIObjectPool.inl
 * @brief Implements the fixed-capacity object pool.
 */
#pragma once

#include <RHI/Memory/RHIObjectPool.hpp>

#include <memory>
#include <utility>

template <typename ObjectType>
Lumen::RHI::TRHIObjectPool<ObjectType>::TRHIObjectPool( const UInt32 InCapacity ) noexcept
    : Objects( MakeUnique<FSlot[]>( InCapacity ) ), Slots( InCapacity )
{
    /* */
}

template <typename ObjectType>
template <typename... ArgumentTypes>
Lumen::TOptional<ObjectType *> Lumen::RHI::TRHIObjectPool<ObjectType>::Create ( ArgumentTypes &&...InArguments ) noexcept
{
    return Slots.Allocate( 1U ).Transform(
        [this, &InArguments...] ( const FIndiceAllocation &InAllocation ) noexcept -> ObjectType *
    {
        FSlot &Slot = Objects[InAllocation.Start];
        return std::construct_at( reinterpret_cast<ObjectType *>( Slot.Storage ), std::forward<ArgumentTypes>( InArguments )... );
    } );
}

template <typename ObjectType> void Lumen::RHI::TRHIObjectPool<ObjectType>::Destroy ( ObjectType *InObject ) noexcept
{
    if ( InObject == nullptr )
    {
        return;
    }

    std::destroy_at( InObject );
    const FSlot *Slot = reinterpret_cast<const FSlot *>( InObject );
    const USize Index = static_cast<USize>( Slot - Objects.Get() );
    Slots.Free( FIndiceAllocation( static_cast<UInt32>( Index ), 1U ) );
}
