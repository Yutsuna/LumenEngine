/**
 * @file TripleBuffer.inl
 * @brief TripleBuffer implementation.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <HAL/Mutex.hpp>

#include <Thread/TripleBuffer.hpp>

/**
 * FBufferFlag
 */

constexpr Lumen::Thread::Internal::FBufferFlag::Type
Lumen::Thread::Internal::FBufferFlag::GetReaderIndex ( FBufferFlag::Type Flags ) noexcept
{
    return ( Flags & ReaderMask ) >> ReaderShift;
}

constexpr Lumen::Thread::Internal::FBufferFlag::Type
Lumen::Thread::Internal::FBufferFlag::GetWriterIndex ( FBufferFlag::Type Flags ) noexcept
{
    return ( Flags & WriterMask ) >> WriterShift;
}

constexpr Lumen::Thread::Internal::FBufferFlag::Type
Lumen::Thread::Internal::FBufferFlag::GetTempIndex ( FBufferFlag::Type Flags ) noexcept
{
    return ( Flags & TempMask ) >> TempShift;
}

constexpr Lumen::Bool Lumen::Thread::Internal::FBufferFlag::IsDirty ( FBufferFlag::Type Flags ) noexcept
{
    return ( Flags & Dirty ) != 0;
}

constexpr Lumen::Thread::Internal::FBufferFlag::Type Lumen::Thread::Internal::FBufferFlag::Make ( FBufferFlag::Type ReaderIndex,
                                                                                                  FBufferFlag::Type WriterIndex,
                                                                                                  FBufferFlag::Type TempIndex,
                                                                                                  Bool bIsDirty ) noexcept
{
    assert( ReaderIndex < 3 and "ReaderIndex must designate one of the 3 triple-buffer slots [0, 2]" );
    assert( WriterIndex < 3 and "WriterIndex must designate one of the 3 triple-buffer slots [0, 2]" );
    assert( TempIndex < 3 and "TempIndex must designate one of the 3 triple-buffer slots [0, 2]" );

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
Lumen::Thread::TTripleBuffer<BufferType>::TTripleBuffer()
    : OwnedBuffers( Lumen::MakeUnique<BufferType[]>( 3 ) ),
      //.
      Buffers( OwnedBuffers.Get() ),
      //.
      BufferFlags( Internal::FBufferFlag::Type::Initial )
{
    Buffers[0] = Buffers[1] = Buffers[2] = BufferType();
}

template <typename BufferType>
Lumen::Thread::TTripleBuffer<BufferType>::TTripleBuffer( const BufferType &InData )
    : OwnedBuffers( Lumen::MakeUnique<BufferType[]>( 3 ) ),
      /**
       *
       */
      Buffers( OwnedBuffers.Get() ),
      /**
       *
       */
      BufferFlags( Internal::FBufferFlag::Type::Initial )
{
    Buffers[0] = Buffers[1] = Buffers[2] = InData;
}

template <typename BufferType>
Lumen::Thread::TTripleBuffer<BufferType>::TTripleBuffer( BufferType ( &InBuffers )[3] )
    : Buffers( &InBuffers[0] ),
      /**
       *
       */
      BufferFlags( ( Internal::FBufferFlag::Type::Initial | Internal::FBufferFlag::Type::Dirty ) )
{
    /** Empty */
}

template <typename BufferType> Lumen::Bool Lumen::Thread::TTripleBuffer<BufferType>::IsDirty () const noexcept
{
    const Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_acquire );

    return Internal::FBufferFlag::IsDirty( CurrentFlags );
}

template <typename BufferType> const BufferType &Lumen::Thread::TTripleBuffer<BufferType>::ReadBuffer () const noexcept
{
    const Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_acquire );
    const Internal::FBufferFlag::Type ReaderIndex  = Internal::FBufferFlag::GetReaderIndex( CurrentFlags );

    return Buffers[ReaderIndex];
}

template <typename BufferType> void Lumen::Thread::TTripleBuffer<BufferType>::SwapReadBuffers () noexcept
{
    Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_acquire );

    if ( not Internal::FBufferFlag::IsDirty( CurrentFlags ) )
    {
        return;
    }

    /*
    - INFO:
    -   Acquire -> synchronizes with the writer's release CAS
    -              Buffers[new ReaderIndex] is fully visible

    -   Release -> publishes that this reader is done with its old slot

     */
    while ( not BufferFlags.compare_exchange_weak( CurrentFlags, SwapReadWithTempFlags( CurrentFlags ), std::memory_order_acq_rel,
                                                   std::memory_order_relaxed ) )
    {
        if ( not Internal::FBufferFlag::IsDirty( CurrentFlags ) )
        {
            /** INFO: Writer changed flags concurrently, dirty bit was cleared. */
            return;
        }
    }
}

template <typename BufferType> void Lumen::Thread::TTripleBuffer<BufferType>::WriteBuffer ( const BufferType &InData )
{
    TLockGuard<FMutex> LockGuard{ WriteMutex };

    const Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_relaxed );
    const Internal::FBufferFlag::Type WriterIndex  = Internal::FBufferFlag::GetWriterIndex( CurrentFlags );

    Buffers[WriterIndex] = InData;

    PublishWrite();
}

template <typename BufferType>
void Lumen::Thread::TTripleBuffer<BufferType>::WriteBuffer ( BufferType &&InData ) noexcept(
    std::is_nothrow_move_assignable_v<BufferType> )
{
    TLockGuard<FMutex> LockGuard{ WriteMutex };

    const Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_relaxed );
    const Internal::FBufferFlag::Type WriterIndex  = Internal::FBufferFlag::GetWriterIndex( CurrentFlags );

    Buffers[WriterIndex] = std::move( InData );

    PublishWrite();
}

template <typename BufferType>
template <typename Callable>
    requires std::is_invocable_v<Callable, BufferType &>
void Lumen::Thread::TTripleBuffer<BufferType>::WriteBuffer ( Callable &&InEditor )
{
    TLockGuard<FMutex> LockGuard{ WriteMutex };

    const Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_relaxed );
    const Internal::FBufferFlag::Type WriterIndex  = Internal::FBufferFlag::GetWriterIndex( CurrentFlags );

    /** INFO: Let the caller modify the underlying buffer in-place directly */
    InEditor( Buffers[WriterIndex] );

    PublishWrite();
}

template <typename BufferType> void Lumen::Thread::TTripleBuffer<BufferType>::SwapWriteBuffers () noexcept
{
    TLockGuard<FMutex> LockGuard{ WriteMutex };
    PublishWrite();
}

template <typename BufferType> const BufferType *Lumen::Thread::TTripleBuffer<BufferType>::GetBuffers () const noexcept
{
    return Buffers;
}

template <typename BufferType>
Lumen::Thread::Internal::FBufferFlag::Type
Lumen::Thread::TTripleBuffer<BufferType>::SwapReadWithTempFlags ( const Internal::FBufferFlag::Type Flags ) const noexcept
{
    /**
     * INFO: Reader takes the Temp slot, Temp takes the old Reader slot.
     * Dirty becomes false because the Reader has just consumed the new data.
     */
    return Internal::FBufferFlag::Make( Internal::FBufferFlag::GetTempIndex( Flags ),   ///<< ReaderIdx
                                        Internal::FBufferFlag::GetWriterIndex( Flags ), ///<< WriterIdx
                                        Internal::FBufferFlag::GetReaderIndex( Flags ), ///<< TempIdx
                                        false );                                        ///<< bIsDirty
}

template <typename BufferType>
Lumen::Thread::Internal::FBufferFlag::Type
Lumen::Thread::TTripleBuffer<BufferType>::SwapWriteWithTempFlags ( const Internal::FBufferFlag::Type Flags ) const noexcept
{
    /**
     * INFO: Writer takes the Temp slot, Temp takes the old Writer slot.
     * Dirty becomes true to signal fresh data to the Reader.
     */
    return Internal::FBufferFlag::Make( Internal::FBufferFlag::GetReaderIndex( Flags ), ///<< ReaderIdx
                                        Internal::FBufferFlag::GetTempIndex( Flags ),   ///<< WriterIdx
                                        Internal::FBufferFlag::GetWriterIndex( Flags ), ///<< TempIdx
                                        true );                                         ///<< bIsDirty
}

template <typename BufferType> void Lumen::Thread::TTripleBuffer<BufferType>::PublishWrite () noexcept
{
    Internal::FBufferFlag::Type CurrentFlags = BufferFlags.load( std::memory_order_relaxed );

    /**
     * INFO:
     *  Acquire -> ensure this thread sees the reader's prior accesses when it swapped away from a slot
     *             needed when the writer later reclaims that slot in the next WriteBuffer call
     *  Release -> publish that new data is available in the writer slot
     */
    while ( not BufferFlags.compare_exchange_weak( CurrentFlags, SwapWriteWithTempFlags( CurrentFlags ),
                                                   std::memory_order_acq_rel, std::memory_order_relaxed ) )
    {
    }
}
