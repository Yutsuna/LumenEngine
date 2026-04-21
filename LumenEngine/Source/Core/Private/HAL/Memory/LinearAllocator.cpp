/**
 * @file LinearAllocator.cpp
 * @brief Implementation of the linear allocator.
 */

#include "HAL/Memory/LinearAllocator.hpp"
#include "Logging/Logger.hpp"

#include <algorithm>
#include <bit>
#include <cassert>

namespace
{

const LumenEngine::FLogCategory LogLinearAllocator( "LinearAllocator" );

/**
 * @brief Aligns a value up to the next multiple of alignment.
 * @param InValue The value to align.
 * @param InAlignment The alignment (must be a power of 2).
 * @return The aligned value.
 */
inline LumenEngine::USize AlignUp ( LumenEngine::USize InValue, LumenEngine::USize InAlignment ) noexcept
{
    assert( std::has_single_bit( InAlignment ) and "Alignment must be a power of 2" );
    return ( InValue + ( InAlignment - 1U ) ) & ~( InAlignment - 1U );
}

} // namespace

LumenEngine::HAL::FLinearAllocator::FLinearAllocator ( void *InBytes, USize InSize ) noexcept
    : Buffer( static_cast<LumenEngine::Byte *>( InBytes ) ), TotalSize( InSize ), Offset( 0U ), HighWatermark( 0U )
{
    /** Ctor */
}

void *LumenEngine::HAL::FLinearAllocator::Allocate ( USize InSize, USize InAlignment ) noexcept
{
    if ( Buffer == nullptr )
    {
        return nullptr;
    }

    const LumenEngine::USize AlignedOffset = AlignUp( Offset, InAlignment );

    /** SECURITY: Check for integer overflow during alignment or if AlignedOffset exceeds TotalSize */
    if ( AlignedOffset < Offset or AlignedOffset > TotalSize )
    {
        return nullptr;
    }

    /** SECURITY: Check for integer overflow during size addition and ensure we stay within bounds */
    if ( InSize > TotalSize - AlignedOffset )
    {
        if ( InSize > TotalSize )
        {
            LUMEN_LOG_WARNING( LogLinearAllocator, "LinearAllocator allocation request ({}) exceeds allocator capacity ({})", InSize, TotalSize );
        }
        else
        {
            LUMEN_LOG_WARNING( LogLinearAllocator, "LinearAllocator out of scratch memory: requested {}, available {}, high watermark {} of {}", InSize,
                               TotalSize - AlignedOffset, HighWatermark, TotalSize );
        }

        return nullptr;
    }

    void *Pointer = Buffer + AlignedOffset;
    Offset        = AlignedOffset + InSize;
    HighWatermark = std::max( Offset, HighWatermark );

    return Pointer;
}

void LumenEngine::HAL::FLinearAllocator::Reset () noexcept
{
    Offset = 0U;
}
