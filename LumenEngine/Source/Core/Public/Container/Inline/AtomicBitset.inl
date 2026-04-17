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
    const USize NewNumBlocks = ( InNumBits + ( BitsPerBlock - 1U ) ) / BitsPerBlock;

    if ( NewNumBlocks != NumBlocks )
    {
        Blocks    = MakeUnique<TAtomic<UInt64>[]>( NewNumBlocks );
        NumBlocks = NewNumBlocks;

        for ( USize Index = 0U; Index < NumBlocks; ++Index )
        {
            Blocks[Index].store( 0ULL, std::memory_order_relaxed );
        }
    }
    else
    {
        ClearAll();
    }

    NumBits = InNumBits;
}

inline void LumenEngine::FAtomicBitset::Set ( USize InIndex ) noexcept
{
    const USize BlockIndex = InIndex / BitsPerBlock;
    const USize BitOffset  = InIndex % BitsPerBlock;
    const UInt64 Mask      = One << BitOffset;

    Blocks[BlockIndex].fetch_or( Mask, std::memory_order_relaxed );
}

inline void LumenEngine::FAtomicBitset::Clear ( USize InIndex ) noexcept
{
    const USize BlockIndex = InIndex / BitsPerBlock;
    const USize BitOffset  = InIndex % BitsPerBlock;
    const UInt64 Mask      = One << BitOffset;

    Blocks[BlockIndex].fetch_and( ~Mask, std::memory_order_relaxed );
}

inline LumenEngine::Bool LumenEngine::FAtomicBitset::Test ( USize InIndex ) const noexcept
{
    const USize BlockIndex = InIndex / BitsPerBlock;
    const USize BitOffset  = InIndex % BitsPerBlock;
    const UInt64 Mask      = One << BitOffset;

    return ( Blocks[BlockIndex].load( std::memory_order_relaxed ) & Mask ) != 0ULL;
}

template <typename Callable>
    requires std::is_invocable_v<Callable, LumenEngine::USize>
inline void LumenEngine::FAtomicBitset::ForEachSetBitAndClear ( Callable &&InFunc ) noexcept
{
    for ( USize BlockIdx = 0U; BlockIdx < NumBlocks; ++BlockIdx )
    {
        /** INFO: Atomic exchange ensures we consume the bits and clear them in one op */
        UInt64 Value = Blocks[BlockIdx].exchange( 0ULL, std::memory_order_relaxed );

        while ( Value != 0ULL )
        {
            const USize BitIdx = static_cast<USize>( std::countr_zero( Value ) );
            InFunc( ( BlockIdx * BitsPerBlock ) + BitIdx );
            Value &= ~( One << BitIdx );
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