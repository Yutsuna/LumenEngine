#pragma once

#include "CoreTypes.hpp"
#include "NonCopyable.hpp"
#include "NonMovable.hpp"

#include "Container/UniquePtr.hpp"

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace LumenEngine
{

namespace Parallel
{

    namespace Internal
    {

        /*
         * @struct FBufferFlag
         * @brief Packed atomic state for TTripleBuffer slot indices and dirty flag.
         */
        struct FBufferFlag
        {
            enum Type : uint8_t
            {
                /** Bits 0-1: reader buffer index (0000 0011) */
                ReaderMask  = ( 1U << 0U ) | ( 1U << 1U ),
                ReaderShift = 0U,

                /** Bits 2-3: writer buffer index (0000 1100) */
                WriterMask  = ( 1U << 2U ) | ( 1U << 3U ),
                WriterShift = 2U,

                /** Bits 4-5: temp swap buffer index (0011 0000) */
                TempMask  = ( 1U << 4U ) | ( 1U << 5U ),
                TempShift = 4U,

                /** Bit 6: new buffer ready for reading (0100 0000) */
                Dirty = 1U << 6U,

                /** Initial state: Reader=2, Writer=1, Temp=0, Dirty=false */
                Initial = ( 2U << ReaderShift ) | ( 1U << WriterShift ) | ( 0U << TempShift )
            };

            static constexpr uint8_t GetReaderIndex ( uint8_t Flags ) noexcept
            {
                return static_cast<uint8_t>( ( Flags & ReaderMask ) >> ReaderShift );
            }

            static constexpr uint8_t GetWriterIndex ( uint8_t Flags ) noexcept
            {
                return static_cast<uint8_t>( ( Flags & WriterMask ) >> WriterShift );
            }

            static constexpr uint8_t GetTempIndex ( uint8_t Flags ) noexcept
            {
                return static_cast<uint8_t>( ( Flags & TempMask ) >> TempShift );
            }

            static constexpr bool IsDirty ( uint8_t Flags ) noexcept
            {
                return ( Flags & Dirty ) != 0;
            }

            static constexpr uint8_t Make ( uint8_t ReaderIndex, uint8_t WriterIndex, uint8_t TempIndex, bool bIsDirty ) noexcept
            {
                assert( ReaderIndex < 3 && "ReaderIndex must designate one of the 3 triple-buffer slots [0, 2]" );
                assert( WriterIndex < 3 && "WriterIndex must designate one of the 3 triple-buffer slots [0, 2]" );
                assert( TempIndex < 3 && "TempIndex must designate one of the 3 triple-buffer slots [0, 2]" );

                const uint8_t ReaderBit = static_cast<uint8_t>( ReaderIndex << ReaderShift );
                const uint8_t WriterBit = static_cast<uint8_t>( WriterIndex << WriterShift );
                const uint8_t TempBit   = static_cast<uint8_t>( TempIndex << TempShift );
                const uint8_t DirtyBit  = bIsDirty ? static_cast<uint8_t>( Dirty ) : 0U;

                return static_cast<uint8_t>( ReaderBit | WriterBit | TempBit | DirtyBit );
            }
        };

    } // namespace Internal

    /**
     * @class TTripleBuffer
     * @brief Triple-buffered latest-snapshot exchange with serialized writers.
     *
     * Slots:
     *  - Reader: where ReadBuffer() reads.
     *  - Writer: where WriteBuffer() writes.
     *  - Temp:   staging slot swapped atomically between reader and writer.
     *
     * Thread safety:
     *  - Multiple concurrent publishers are serialized via an internal write-side spinlock.
     *  - Exactly one read-owner thread must drive SwapReadBuffers() for a given instance.
     *  - Any number of threads may inspect the current reader snapshot through ReadBuffer(),
     *    but the returned reference is immutable and only stable until the next reader-driven swap.
     *
     * @tparam BufferType The type stored in each slot.
     */
    template <typename BufferType> class TTripleBuffer : public FNonCopyable, public FNonMovable
    {
        static_assert( std::is_default_constructible_v<BufferType>, "BufferType must be default constructible because all triple-buffer slots are pre-allocated" );

    public:

        TTripleBuffer ();

        /**
         * @brief Initializes all three slots with a copy of @p InData.
         */
        explicit TTripleBuffer ( const BufferType &InData );

        /**
         * @brief Wraps an existing three-element array (caller owns the memory).
         * The caller must keep InBuffers alive for the lifetime of this instance.
         *
         * Index mapping:
         *  - 0: Temp slot
         *  - 1: Writer slot
         *  - 2: Reader slot
         */
        explicit TTripleBuffer ( BufferType ( &InBuffers )[3] );

        ~TTripleBuffer () = default;

        //------

        /**
         * @return True if the writer has published new data not yet consumed by the reader.
         */
        [[nodiscard]] bool IsDirty () const noexcept;

        //------

        /** @return Immutable reference to the current reader snapshot.
         * @note Call SwapReadBuffers() first to expose the freshest published snapshot.
         */
        [[nodiscard]] const BufferType &ReadBuffer () const noexcept;

        /**
         * @brief Atomically promotes the latest writer-published slot to the reader slot.
         * Noop if no new data is available ( IsDirty() == false ).
         */
        void SwapReadBuffers () noexcept;

        /**
         * @brief Writes @p InData to the writer slot and publishes it atomically.
         * Safe to call from multiple producer threads simultaneously.
         */
        void WriteBuffer ( const BufferType &InData );

        /**
         * @brief Writes @p InData to the writer slot via move and publishes it atomically.
         * Safe to call from multiple producer threads simultaneously.
         */
        void WriteBuffer ( BufferType &&InData ) noexcept( std::is_nothrow_move_assignable_v<BufferType> );

        /**
         * @brief Publishes the current writer slot without writing.
         * Safe to call from multiple producer threads simultaneously.
         */
        void SwapWriteBuffers () noexcept;

    private:

        [[nodiscard]] UInt8 SwapReadWithTempFlags ( UInt8 Flags ) const noexcept;
        [[nodiscard]] UInt8 SwapWriteWithTempFlags ( UInt8 Flags ) const noexcept;

        void PublishWrite () noexcept;
        void AcquireWriteLock () noexcept;
        void ReleaseWriteLock () noexcept;

    private:

        struct FWriteLockGuard
        {
            TTripleBuffer *Owner;
            ~FWriteLockGuard ()
            {
                Owner->ReleaseWriteLock();
            }
        };

    private:

        TUniquePtr<BufferType[]> OwnedBuffers;
        BufferType *Buffers = nullptr;
        TAtomic<UInt8> BufferFlags;
        alignas( 64 ) FAtomicFlag WriteGuard;
    };

} // namespace Parallel

} // namespace LumenEngine

#include "Inline/TripleBuffer.inl"
