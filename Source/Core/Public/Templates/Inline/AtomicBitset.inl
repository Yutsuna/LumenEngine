/**
 * @file AtomicBitset.inl
 * @brief Implementation of the FAtomicBitset class.
 */

#pragma once

#include <Algo/ForEach.hpp>
#include <Core/CoreTypes.hpp>
#include <Memory/Span.hpp>
#include <Templates/AtomicBitset.hpp>

#include <bit>
#include <iterator>
#include <span>

/**
 * Public
 */

Lumen::FAtomicBitset::FAtomicBitset ( USize InNumBits )
{
    Resize( InNumBits );
}

void Lumen::FAtomicBitset::Set ( USize InIndex ) noexcept
{
    LUMEN_ASSERT( InIndex < NumBits );

    const USize BlockIndex = InIndex / BitsPerBlock;
    const USize BitOffset  = InIndex % BitsPerBlock;
    const UInt64 Mask      = One << BitOffset;

    /** INFO: Release pairs with the acquire in ForEachSetBitAndClear: data written before Set()
     *       becomes visible to the thread that consumes the dirty bit. */
    std::next( Blocks.Get(), static_cast<IPtr>( BlockIndex ) )->fetch_or( Mask, std::memory_order_release );
}

void Lumen::FAtomicBitset::Clear ( USize InIndex ) noexcept
{
    LUMEN_ASSERT( InIndex < NumBits );

    const USize BlockIndex = InIndex / BitsPerBlock;
    const USize BitOffset  = InIndex % BitsPerBlock;
    const UInt64 Mask      = One << BitOffset;

    std::next( Blocks.Get(), static_cast<IPtr>( BlockIndex ) )->fetch_and( ~Mask, std::memory_order_relaxed );
}

Lumen::Bool Lumen::FAtomicBitset::Test ( USize InIndex ) const noexcept
{
    LUMEN_ASSERT( InIndex < NumBits );

    const USize BlockIndex = InIndex / BitsPerBlock;
    const USize BitOffset  = InIndex % BitsPerBlock;
    const UInt64 Mask      = One << BitOffset;

    return ( std::next( Blocks.Get(), static_cast<IPtr>( BlockIndex ) )->load( std::memory_order_acquire ) & Mask ) != 0ULL;
}

template <typename Callable>
    requires Lumen::Concepts::CIsInvocable<Callable, Lumen::USize>
void Lumen::FAtomicBitset::ForEachSetBitAndClear ( Callable &&InFunc ) noexcept
{
    USize BlockIdx = 0U;
    Algo::ForEach( TSpan{ Blocks.Get(), NumBlocks }, [&] ( TAtomic<UInt64> &Block )
    {
        /** INFO: Atomic exchange ensures we consume the bits and clear them in one op.
         *       Acquire pairs with the release in Set() so the consumer sees the data that was
         *       marked dirty, not just the flag. */
        UInt64 Value = Block.exchange( 0ULL, std::memory_order_acquire );

        while ( Value != 0ULL )
        {
            const USize BitIdx = static_cast<USize>( std::countr_zero( Value ) );
            InFunc( ( BlockIdx * BitsPerBlock ) + BitIdx );
            Value &= ~( One << BitIdx );
        }

        ++BlockIdx;
    } );
}

void Lumen::FAtomicBitset::ClearAll () noexcept
{
    Algo::ForEach( TSpan{ Blocks.Get(), NumBlocks }, [] ( TAtomic<UInt64> &Block )
    {
        Block.store( 0ULL, std::memory_order_relaxed );
    } );
}

LUMEN_FORCE_INLINE Lumen::USize Lumen::FAtomicBitset::GetNumBits () const noexcept
{
    return NumBits;
}
