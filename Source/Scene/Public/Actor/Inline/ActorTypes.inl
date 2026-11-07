/**
 * @file ActorTypes.inl
 * @brief Inline implementation of actor types.
 */

#pragma once

#include <Actor/ActorTypes.hpp>

constexpr Lumen::UInt32 Lumen::ActorIdSlotIndex ( const ActorID InId ) noexcept
{
    return static_cast<UInt32>( InId & 0xFFFFFFFFULL );
}

constexpr Lumen::UInt32 Lumen::ActorIdGeneration ( const ActorID InId ) noexcept
{
    return static_cast<UInt32>( InId >> 32 );
}

constexpr Lumen::ActorID Lumen::MakeActorId ( const UInt32 InSlotIndex, const UInt32 InGeneration ) noexcept
{
    return ( static_cast<ActorID>( InGeneration ) << 32 ) | static_cast<ActorID>( InSlotIndex );
}
