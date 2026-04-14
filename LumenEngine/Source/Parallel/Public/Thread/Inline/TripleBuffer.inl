#pragma once

#include "CoreTypes.hpp"
#include "Thread/Mutex.hpp"
#include "Thread/TripleBuffer.hpp"

/**
 * FBufferFlag
 */

constexpr LumenEngine::Parallel::Internal::FBufferFlag::Type LumenEngine::Parallel::Internal::FBufferFlag::GetReaderIndex ( FBufferFlag::Type Flags ) noexcept
{
    return ( Flags & ReaderMask ) >> ReaderShift;
}

constexpr LumenEngine::Parallel::Internal::FBufferFlag::Type LumenEngine::Parallel::Internal::FBufferFlag::GetWriterIndex ( FBufferFlag::Type Flags ) noexcept
{
    return ( Flags & WriterMask ) >> WriterShift;
}

constexpr LumenEngine::Parallel::Internal::FBufferFlag::Type LumenEngine::Parallel::Internal::FBufferFlag::GetTempIndex ( FBufferFlag::Type Flags ) noexcept
{
    return ( Flags & TempMask ) >> TempShift;
}

constexpr LumenEngine::Bool LumenEngine::Parallel::Internal::FBufferFlag::IsDirty ( FBufferFlag::Type Flags ) noexcept
{
    return ( Flags & Dirty ) != 0;
}

constexpr LumenEngine::Parallel::Internal::FBufferFlag::Type
LumenEngine::Parallel::Internal::FBufferFlag::Make ( FBufferFlag::Type ReaderIndex, FBufferFlag::Type WriterIndex, FBufferFlag::Type TempIndex, Bool bIsDirty ) noexcept
{
    assert( ReaderIndex < 3 && "ReaderIndex must designate one of the 3 triple-buffer slots [0, 2]" );
    assert( WriterIndex < 3 && "WriterIndex must designate one of the 3 triple-buffer slots [0, 2]" );
    assert( TempIndex < 3 && "TempIndex must designate one of the 3 triple-buffer slots [0, 2]" );

    const FBufferFlag::Type ReaderBit = ReaderIndex << ReaderShift;
    const FBufferFlag::Type WriterBit = WriterIndex << WriterShift;
    const FBufferFlag::Type TempBit   = TempIndex << TempShift;
    const FBufferFlag::Type DirtyBit  = bIsDirty ? Dirty : static_cast<FBufferFlag::Type>( 0U );

    return ReaderBit | WriterBit | TempBit | DirtyBit;
}

/**
 * TTripleBuffer
 */

template <typename BufferType>
LumenEngine::Parallel::TTripleBuffer<BufferType>::TTripleBuffer()
    : OwnedBuffers( LumenEngine::MakeUnique<BufferType[]>( 3 ) ),
      //
      Buffers( OwnedBuffers.Get() ),
      //
      BufferFlags( Internal::FBufferFlag::Type::Initial )
{
    Buffers[0] = Buffers[1] = Buffers[2] = BufferType();
}

template <typename BufferType>
LumenEngine::Parallel::TTripleBuffer<BufferType>::TTripleBuffer( const BufferType &InData )
    : OwnedBuffers( LumenEngine::MakeUnique<BufferType[]>( 3 ) ),
      //
      Buffers( OwnedBuffers.Get() ),
      //
      BufferFlags( Internal::FBufferFlag::Type::Initial )
{
    Buffers[0] = Buffers[1] = Buffers[2] = InData;
}

template <typename BufferType>
LumenEngine::Parallel::TTripleBuffer<BufferType>::TTripleBuffer( BufferType ( &InBuffers )[3] )
    : Buffers( &InBuffers[0] ),
      //
      BufferFlags( ( Internal::FBufferFlag::Type::Initial | Internal::FBufferFlag::Type::Dirty ) )
{
}

template <typename BufferType> LumenEngine::Bool LumenEngine::Parallel::TTripleBuffer<BufferType>::IsDirty () const noexcept
{
    const Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_acquire );

    return Internal::FBufferFlag::IsDirty( CurrentFlags );
}

template <typename BufferType> const BufferType &LumenEngine::Parallel::TTripleBuffer<BufferType>::ReadBuffer () const noexcept
{
    const Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_acquire );
    const Internal::FBufferFlag::Type ReaderIndex  = Internal::FBufferFlag::GetReaderIndex( CurrentFlags );

    return Buffers[ReaderIndex];
}

template <typename BufferType> void LumenEngine::Parallel::TTripleBuffer<BufferType>::SwapReadBuffers () noexcept
{
    Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_acquire );

    if ( not Internal::FBufferFlag::IsDirty( CurrentFlags ) )
    {
        return;
    }

    /** INFO:
     *   Acquire -> synchronizes with the writer's release CAS
     *              Buffers[new ReaderIndex] is fully visible
     *
     *   Release -> publishes that this reader is done with its old slot
     */
    while ( not BufferFlags.compare_exchange_weak( CurrentFlags, SwapReadWithTempFlags( CurrentFlags ), std::memory_order_acq_rel, std::memory_order_relaxed ) )
    {
        if ( not Internal::FBufferFlag::IsDirty( CurrentFlags ) )
        {
            /** INFO: Writer changed flags concurrently, dirty bit was cleared. */
            return;
        }
    }
}

template <typename BufferType> void LumenEngine::Parallel::TTripleBuffer<BufferType>::WriteBuffer ( const BufferType &InData )
{
    TLockGuard<FMutex> LockGuard{ WriteMutex };

    const Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_relaxed );
    const Internal::FBufferFlag::Type WriterIndex  = Internal::FBufferFlag::GetWriterIndex( CurrentFlags );

    Buffers[WriterIndex] = InData;

    PublishWrite();
}

template <typename BufferType>
void LumenEngine::Parallel::TTripleBuffer<BufferType>::WriteBuffer ( BufferType &&InData ) noexcept( std::is_nothrow_move_assignable_v<BufferType> )
{
    TLockGuard<FMutex> LockGuard{ WriteMutex };

    const Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_relaxed );
    const Internal::FBufferFlag::Type WriterIndex  = Internal::FBufferFlag::GetWriterIndex( CurrentFlags );

    Buffers[WriterIndex] = std::move( InData );

    PublishWrite();
}

template <typename BufferType> void LumenEngine::Parallel::TTripleBuffer<BufferType>::SwapWriteBuffers () noexcept
{
    TLockGuard<FMutex> LockGuard{ WriteMutex };
    PublishWrite();
}

template <typename BufferType>
LumenEngine::Parallel::Internal::FBufferFlag::Type
LumenEngine::Parallel::TTripleBuffer<BufferType>::SwapReadWithTempFlags ( const Internal::FBufferFlag::Type Flags ) const noexcept
{
    /**
     * INFO: Reader takes the Temp slot, Temp takes the old Reader slot.
     * Dirty becomes false because the Reader has just consumed the new data.
     */
    return Internal::FBufferFlag::Make(
        /* ReaderIdx */ Internal::FBufferFlag::GetTempIndex( Flags ),
        /* WriterIdx */ Internal::FBufferFlag::GetWriterIndex( Flags ),
        /* TempIdx   */ Internal::FBufferFlag::GetReaderIndex( Flags ),
        /* bIsDirty  */ false );
}

template <typename BufferType>
LumenEngine::Parallel::Internal::FBufferFlag::Type
LumenEngine::Parallel::TTripleBuffer<BufferType>::SwapWriteWithTempFlags ( const Internal::FBufferFlag::Type Flags ) const noexcept
{
    /**
     * INFO: Writer takes the Temp slot, Temp takes the old Writer slot.
     * Dirty becomes true to signal fresh data to the Reader.
     */
    return Internal::FBufferFlag::Make(
        /* ReaderIdx */ Internal::FBufferFlag::GetReaderIndex( Flags ),
        /* WriterIdx */ Internal::FBufferFlag::GetTempIndex( Flags ),
        /* TempIdx   */ Internal::FBufferFlag::GetWriterIndex( Flags ),
        /* bIsDirty  */ true );
}

template <typename BufferType> void LumenEngine::Parallel::TTripleBuffer<BufferType>::PublishWrite () noexcept
{
    Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_relaxed );

    /**
     * INFO:
     *  Acquire -> ensure this thread sees the reader's prior accesses when it swapped away from a slot
     *             needed when the writer later reclaims that slot in the next WriteBuffer call
     *  Release -> publish that new data is available in the writer slot
     */
    while ( not BufferFlags.compare_exchange_weak( CurrentFlags, SwapWriteWithTempFlags( CurrentFlags ), std::memory_order_acq_rel, std::memory_order_relaxed ) )
    {
    }
}
