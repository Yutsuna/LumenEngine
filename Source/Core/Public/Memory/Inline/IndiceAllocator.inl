/**
 * @file IndiceAllocator.inl
 * @brief Inline implementation of the IndiceAllocator classes.
 */

#pragma once

#include <Memory/IndiceAllocator.hpp>

/**
 * FIndiceAllocatorRange
 */

Lumen::FIndiceAllocatorRange::FIndiceAllocatorRange ( UInt32 InFirst, UInt32 InLast ) noexcept : First( InFirst ), Last( InLast )
{
    /* */
}

/**
 * FIndiceAllocation
 */

Lumen::FIndiceAllocation::FIndiceAllocation ( UInt32 InStart, UInt32 InCount ) noexcept : Start( InStart ), Count( InCount )
{
    /* */
}

/**
 * FIndiceAllocator
 */

Lumen::FIndiceAllocator::FIndiceAllocator ( UInt32 InNumDescriptors ) noexcept : Capacity( InNumDescriptors )
{
    if ( InNumDescriptors > 0U )
    {
        Ranges.emplace_back( 0U, InNumDescriptors - 1U );
    }
}

Lumen::UInt32 Lumen::FIndiceAllocator::GetCapacity () const noexcept
{
    return Capacity;
}

/**
 * FIndiceHeapAllocator
 */

Lumen::FIndiceHeapAllocator::FIndiceHeapAllocator ( UInt32 InCapacity ) noexcept : Capacity( InCapacity )
{
    if ( InCapacity == 0U )
    {
        return;
    }

    /** INFO: Overflow-safe calculation of the number of 64-bit words. */
    const UInt32 NumWords = ( InCapacity / BitsPerWord ) + static_cast<UInt32>( ( InCapacity % BitsPerWord ) != 0U );

    Bitmap.assign( NumWords, 0ULL );

    /**
     * INFO: Mark unused bits of the last word as allocated.
     *       Only the lower ValidBits are available for allocation.
     */
    const UInt32 ValidBits = InCapacity % BitsPerWord;

    if ( ValidBits != 0U )
    {
        Bitmap[NumWords - 1U] = ~( ( 1ULL << ValidBits ) - 1ULL );
    }
}

Lumen::UInt32 Lumen::FIndiceHeapAllocator::GetCapacity () const noexcept
{
    return Capacity;
}
