/**
 * @file AtomicBitset.inl
 * @brief Implementation of the FAtomicBitset class.
 */

#pragma once

#include "Container/AtomicBitset.hpp"

#include <bit>

inline LumenEngine::FAtomicBitset::FAtomicBitset ( USize InNumBits )
{
    Resize( InNumBits );
}

inline void LumenEngine::FAtomicBitset::Resize ( USize InNumBits )
{
    const USize NewNumBlocks = ( InNumBits + 63U ) / 64U;

    if ( NewNumBlocks != NumBlocks )
    {
        TUniquePtr<TAtomic<UInt64>[]> NewBlocks = MakeUnique<TAtomic<UInt64>[]>( NewNumBlocks );

        for ( USize Index = 0U; Index < NewNumBlocks; ++Index )
        {
            /** NOTE: TAtomic is not Copyable / Movable, so we must use Store/Load */
            if ( Index < NumBlocks )
            {
                NewBlocks[Index].store( Blocks[Index].load( std::memory_order_relaxed ), std::memory_order_relaxed );
            }
            else
            {
                NewBlocks[Index].store( 0ULL, std::memory_order_relaxed );
            }
        }

        Blocks    = std::move( NewBlocks );
        NumBlocks = NewNumBlocks;
    }

    NumBits = InNumBits;
}

inline void LumenEngine::FAtomicBitset::Set ( USize InIndex ) noexcept
{
    const USize BlockIndex = InIndex / 64U;
    const USize BitOffset  = InIndex % 64U;
    const UInt64 Mask      = 1ULL << BitOffset;

    Blocks[BlockIndex].fetch_or( Mask, std::memory_order_relaxed );
}

inline void LumenEngine::FAtomicBitset::Clear ( USize InIndex ) noexcept
{
    const USize BlockIndex = InIndex / 64U;
    const USize BitOffset  = InIndex % 64U;
    const UInt64 Mask      = 1ULL << BitOffset;

    Blocks[BlockIndex].fetch_and( ~Mask, std::memory_order_relaxed );
}

inline LumenEngine::Bool LumenEngine::FAtomicBitset::Test ( USize InIndex ) const noexcept
{
    const USize BlockIndex = InIndex / 64U;
    const USize BitOffset  = InIndex % 64U;
    const UInt64 Mask      = 1ULL << BitOffset;

    return ( Blocks[BlockIndex].load( std::memory_order_relaxed ) & Mask ) != 0ULL;
}

template <typename Callable>
    requires std::is_invocable_v<Callable, LumenEngine::USize>
inline void LumenEngine::FAtomicBitset::ForEachSetBitAndClear ( Callable &&InFunc ) noexcept
{
    for ( USize BlockIdx = 0U; BlockIdx < NumBlocks; ++BlockIdx )
    {
        UInt64 Value = Blocks[BlockIdx].exchange( 0ULL, std::memory_order_relaxed );

        while ( Value != 0ULL )
        {
            const USize BitIdx = static_cast<USize>( std::countr_zero( Value ) );
            InFunc( ( BlockIdx * 64U ) + BitIdx );
            Value &= ~( 1ULL << BitIdx );
        }
    }
}

inline void LumenEngine::FAtomicBitset::ClearAll () noexcept
{
    for ( USize Index = 0U; Index < NumBlocks; ++Index )
    {
        Blocks[Index].store( 0ULL, std::memory_order_relaxed );
    }
}