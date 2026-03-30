#pragma once

#include "Thread/TripleBuffer.hpp"

template <typename BufferType>
LumenEngine::Parallel::TTripleBuffer<BufferType>::TTripleBuffer ()
    : OwnedBuffers( LumenEngine::MakeUnique<BufferType[]>( 3 ) ),
      //
      Buffers( OwnedBuffers.Get() ),
      //
      BufferFlags( LumenEngine::Parallel::Internal::FBufferFlag::Type::Initial )
{
    Buffers[0] = Buffers[1] = Buffers[2] = BufferType();
}

template <typename BufferType>
LumenEngine::Parallel::TTripleBuffer<BufferType>::TTripleBuffer ( const BufferType &InData )
    : OwnedBuffers( LumenEngine::MakeUnique<BufferType[]>( 3 ) ),
      //
      Buffers( OwnedBuffers.Get() ),
      //
      BufferFlags( LumenEngine::Parallel::Internal::FBufferFlag::Type::Initial )
{
    Buffers[0] = Buffers[1] = Buffers[2] = InData;
}

template <typename BufferType>
LumenEngine::Parallel::TTripleBuffer<BufferType>::TTripleBuffer ( BufferType ( &InBuffers )[3] )
    : Buffers( &InBuffers[0] ),
      //
      BufferFlags( static_cast<UInt8>( LumenEngine::Parallel::Internal::FBufferFlag::Type::Initial | LumenEngine::Parallel::Internal::FBufferFlag::Type::Dirty ) )
{
}

template <typename BufferType> bool LumenEngine::Parallel::TTripleBuffer<BufferType>::IsDirty () const noexcept
{
    const UInt8 CurrentFlags = BufferFlags.load( std::memory_order_acquire );

    return LumenEngine::Parallel::Internal::FBufferFlag::IsDirty( CurrentFlags );
}

template <typename BufferType> const BufferType &LumenEngine::Parallel::TTripleBuffer<BufferType>::ReadBuffer () const noexcept
{
    const UInt8 CurrentFlags = BufferFlags.load( std::memory_order_acquire );
    const UInt8 ReaderIndex  = LumenEngine::Parallel::Internal::FBufferFlag::GetReaderIndex( CurrentFlags );

    return Buffers[ReaderIndex];
}

template <typename BufferType> void LumenEngine::Parallel::TTripleBuffer<BufferType>::SwapReadBuffers () noexcept
{
    UInt8 CurrentFlags = BufferFlags.load( std::memory_order_acquire );

    if ( not LumenEngine::Parallel::Internal::FBufferFlag::IsDirty( CurrentFlags ) )
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
        if ( not LumenEngine::Parallel::Internal::FBufferFlag::IsDirty( CurrentFlags ) )
        {
            /** INFO: Writer changed flags concurrently, dirty bit was cleared. */
            return;
        }
    }
}

template <typename BufferType> void LumenEngine::Parallel::TTripleBuffer<BufferType>::WriteBuffer ( const BufferType &InData )
{
    AcquireWriteLock();
    FWriteLockGuard LockGuard{ this };

    const UInt8 CurrentFlags = BufferFlags.load( std::memory_order_relaxed );
    const UInt8 WriterIndex  = LumenEngine::Parallel::Internal::FBufferFlag::GetWriterIndex( CurrentFlags );

    Buffers[WriterIndex] = InData;

    PublishWrite();
}

template <typename BufferType>
void LumenEngine::Parallel::TTripleBuffer<BufferType>::WriteBuffer ( BufferType &&InData ) noexcept( std::is_nothrow_move_assignable_v<BufferType> )
{
    AcquireWriteLock();
    FWriteLockGuard LockGuard{ this };

    const UInt8 CurrentFlags = BufferFlags.load( std::memory_order_relaxed );
    const UInt8 WriterIndex  = LumenEngine::Parallel::Internal::FBufferFlag::GetWriterIndex( CurrentFlags );

    Buffers[WriterIndex] = std::move( InData );

    PublishWrite();
}

template <typename BufferType> void LumenEngine::Parallel::TTripleBuffer<BufferType>::SwapWriteBuffers () noexcept
{
    AcquireWriteLock();
    FWriteLockGuard LockGuard{ this };
    PublishWrite();
}

template <typename BufferType> LumenEngine::UInt8 LumenEngine::Parallel::TTripleBuffer<BufferType>::SwapReadWithTempFlags ( const UInt8 Flags ) const noexcept
{
    /**
     * INFO: Reader takes the Temp slot, Temp takes the old Reader slot.
     * Dirty becomes false because the Reader has just consumed the new data.
     */
    return LumenEngine::Parallel::Internal::FBufferFlag::Make(
        /* ReaderIdx */ LumenEngine::Parallel::Internal::FBufferFlag::GetTempIndex( Flags ),
        /* WriterIdx */ LumenEngine::Parallel::Internal::FBufferFlag::GetWriterIndex( Flags ),
        /* TempIdx   */ LumenEngine::Parallel::Internal::FBufferFlag::GetReaderIndex( Flags ),
        /* bIsDirty  */ false );
}

template <typename BufferType> LumenEngine::UInt8 LumenEngine::Parallel::TTripleBuffer<BufferType>::SwapWriteWithTempFlags ( const UInt8 Flags ) const noexcept
{
    /**
     * INFO: Writer takes the Temp slot, Temp takes the old Writer slot.
     * Dirty becomes true to signal fresh data to the Reader.
     */
    return LumenEngine::Parallel::Internal::FBufferFlag::Make(
        /* ReaderIdx */ LumenEngine::Parallel::Internal::FBufferFlag::GetReaderIndex( Flags ),
        /* WriterIdx */ LumenEngine::Parallel::Internal::FBufferFlag::GetTempIndex( Flags ),
        /* TempIdx   */ LumenEngine::Parallel::Internal::FBufferFlag::GetWriterIndex( Flags ),
        /* bIsDirty  */ true );
}

template <typename BufferType> void LumenEngine::Parallel::TTripleBuffer<BufferType>::AcquireWriteLock () noexcept
{
    while ( WriteGuard.test_and_set( std::memory_order_acquire ) )
    {
        WriteGuard.wait( true, std::memory_order_relaxed );
    }
}

template <typename BufferType> void LumenEngine::Parallel::TTripleBuffer<BufferType>::ReleaseWriteLock () noexcept
{
    WriteGuard.clear( std::memory_order_release );
    WriteGuard.notify_one();
}

template <typename BufferType> void LumenEngine::Parallel::TTripleBuffer<BufferType>::PublishWrite () noexcept
{
    UInt8 CurrentFlags = BufferFlags.load( std::memory_order_relaxed );

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
