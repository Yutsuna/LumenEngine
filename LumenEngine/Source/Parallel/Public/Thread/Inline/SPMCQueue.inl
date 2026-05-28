/**
 * @file SPMCQueue.inl
 * @brief Inline implementation for TSPMCQueue.
 */

#pragma once

#include "Thread/SPMCQueue.hpp"

template <typename Type>
LumenEngine::Parallel::TSPMCQueue<Type>::TSPMCQueue( USize InCapacity )
    : OwnedBuffer( LumenEngine::MakeUnique<FCell[]>( InCapacity ) ),
      /* */
      CapacityMask( InCapacity - 1ULL ),
      /* */
      Buffer( OwnedBuffer.Get() ),
      /* */
      DequeuePosition( 0ULL )
{
    [[assume( InCapacity >= 2ULL && ( ( InCapacity & ( InCapacity - 1ULL ) ) == 0ULL ) )]];

    for ( USize Index = 0ULL; Index < InCapacity; ++Index )
    {
        Buffer[Index].Sequence.store( Index, std::memory_order_relaxed );
    }
}

template <typename Type> LumenEngine::Bool LumenEngine::Parallel::TSPMCQueue<Type>::Push ( const Type &InData ) noexcept
{
    FCell *const Cell    = &Buffer[EnqueuePosition & CapacityMask];
    const USize Sequence = Cell->Sequence.load( std::memory_order_acquire );
    const IPtr Diff      = static_cast<IPtr>( Sequence ) - static_cast<IPtr>( EnqueuePosition );

    if ( Diff == 0 )
    {
        Cell->Data = InData;
        Cell->Sequence.store( EnqueuePosition + 1ULL, std::memory_order_release );
        ++EnqueuePosition;
        return true;
    }

    return false;
}

template <typename Type> LumenEngine::Bool LumenEngine::Parallel::TSPMCQueue<Type>::Push ( Type &&InData ) noexcept
{
    FCell *const Cell    = &Buffer[EnqueuePosition & CapacityMask];
    const USize Sequence = Cell->Sequence.load( std::memory_order_acquire );
    const IPtr Diff      = static_cast<IPtr>( Sequence ) - static_cast<IPtr>( EnqueuePosition );

    if ( Diff == 0 )
    {
        Cell->Data = std::move( InData );
        Cell->Sequence.store( EnqueuePosition + 1ULL, std::memory_order_release );
        ++EnqueuePosition;
        return true;
    }

    return false;
}

template <typename Type> LumenEngine::TOptional<Type> LumenEngine::Parallel::TSPMCQueue<Type>::Pop () noexcept
{
    USize Pos   = DequeuePosition.load( std::memory_order_relaxed );
    FCell *Cell = nullptr;

    for ( ;; )
    {
        Cell                 = &Buffer[Pos & CapacityMask];
        const USize Sequence = Cell->Sequence.load( std::memory_order_acquire );
        const IPtr Diff      = static_cast<IPtr>( Sequence ) - static_cast<IPtr>( Pos + 1ULL );

        if ( Diff == 0 )
        {
            if ( DequeuePosition.compare_exchange_weak( Pos, Pos + 1ULL, std::memory_order_relaxed ) )
            {
                break;
            }
        }
        else if ( Diff < 0 )
        {
            return {};
        }
        else
        {
            Pos = DequeuePosition.load( std::memory_order_relaxed );
        }
    }

    Type Result = std::move( Cell->Data );
    Cell->Sequence.store( Pos + CapacityMask + 1ULL, std::memory_order_release );

    return Result;
}
