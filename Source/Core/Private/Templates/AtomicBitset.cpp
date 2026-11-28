/**
 * @file AtomicBitset.cpp
 * @brief Implementation of Lumen::FAtomicBitset.
 */

#include <Templates/AtomicBitset.hpp>

/**
 * Public
 */

void Lumen::FAtomicBitset::Resize ( USize InNumBits )
{
    const USize NewNumBlocks = ( InNumBits + ( BitsPerBlock - 1U ) ) / BitsPerBlock;

    if ( NewNumBlocks != NumBlocks )
    {
        Blocks    = MakeUnique<TAtomic<UInt64>[]>( NewNumBlocks );
        NumBlocks = NewNumBlocks;

        TAtomic<UInt64> *const BlockData = Blocks.Get();

        for ( USize Index = 0U; Index < NumBlocks; ++Index )
        {
            BlockData[Index].store( 0ULL, std::memory_order_relaxed );
        }
    }
    else
    {
        ClearAll();
    }

    NumBits = InNumBits;
}
