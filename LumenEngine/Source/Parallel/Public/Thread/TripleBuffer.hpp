#pragma once

#include "CoreTypes.hpp"
#include "EnumFlags.hpp"
#include "NonCopyable.hpp"
#include "NonMovable.hpp"
#include "Thread/Mutex.hpp"

#include "Container/UniquePtr.hpp"

#include <cassert>
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
            enum Type : UInt8
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

            static constexpr Type GetReaderIndex ( Type Flags ) noexcept;
            static constexpr Type GetWriterIndex ( Type Flags ) noexcept;
            static constexpr Type GetTempIndex ( Type Flags ) noexcept;
            static constexpr Bool IsDirty ( Type Flags ) noexcept;
            static constexpr Type Make ( Type ReaderIndex, Type WriterIndex, Type TempIndex, Bool bIsDirty ) noexcept;
        };

        LUMEN_ENUM_FLAGS( FBufferFlag::Type );

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
    template <typename BufferType> class TTripleBuffer final : public FNonCopyable, public FNonMovable
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
        [[nodiscard]] Bool IsDirty () const noexcept;

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

    private:

        TUniquePtr<BufferType[]> OwnedBuffers;
        BufferType *Buffers = nullptr;
        TAtomic<Internal::FBufferFlag::Type> BufferFlags;
        alignas( 64 ) FMutex WriteMutex;
    };

} // namespace Parallel

} // namespace LumenEngine

#include "Inline/TripleBuffer.inl"
