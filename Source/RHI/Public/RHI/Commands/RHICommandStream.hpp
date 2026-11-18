/**
 * @file RHICommandStream.hpp
 * @brief Declares aligned backend-neutral command storage.
 */

#pragma once

#include "Concepts/TriviallyCopyable.hpp"
#include <Memory/SharedPtr.hpp>
#include <Memory/Span.hpp>
#include <RHI/Types/IRHIObject.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHICommandView
     * @brief One immutable opcode and payload view.
     */
    struct FRHICommandView
    {
        UInt32 Opcode = 0U;
        TSpan<const Byte> Payload;
    };

    /**
     * @class FRHICommandStream
     * @brief Chunked aligned commands plus retained object references.
     */
    class RHI_EXPORT FRHICommandStream final
    {
    public:

        /**
         * @brief Creates a stream with a fixed chunk capacity.
         * @param InChunkCapacity Byte capacity of each storage chunk.
         */
        explicit FRHICommandStream ( USize InChunkCapacity = 4096U ) noexcept;

        /** @brief Appends a raw payload; a command never crosses a chunk boundary. */
        TExpected<void, FRHIError> WritePayload ( UInt32 InOpcode, TSpan<const Byte> InPayload ) noexcept;

        /** @brief Appends one trivially copyable payload. */
        template <typename PayloadType>
            requires Concepts::CTriviallyCopyable<PayloadType>
        TExpected<void, FRHIError> Write ( UInt32 InOpcode, const PayloadType &InPayload ) noexcept;

        /** @brief Reads and validates one command by index. */
        [[nodiscard]] TExpected<FRHICommandView, FRHIError> Read ( USize InIndex ) const noexcept;

        /** @brief Decodes one typed command after checking opcode and payload size. */
        template <typename PayloadType>
            requires Concepts::CTriviallyCopyable<PayloadType>
        [[nodiscard]] TExpected<PayloadType, FRHIError> ReadPayload ( USize InIndex, UInt32 InExpectedOpcode ) const noexcept;

        /** @brief Retains an object until Clear or stream destruction. */
        void Retain ( TSharedPtr<IRHIObject> InObject ) noexcept;

        /**
         * @brief Keeps an object alive until Clear or stream destruction.
         * @param InObject Object referenced by a recorded command.
         */
        template <typename ObjectType> void Retain ( const TSharedRef<ObjectType> &InObject ) noexcept;

        /**
         * @brief Drops all recorded commands and releases retained object references.
         */
        void Clear () noexcept;

        /**
         * @brief Returns the number of recorded commands.
         * @return Record count.
         */
        [[nodiscard]] USize GetCommandCount () const noexcept;

        /**
         * @brief Returns the number of allocated storage chunks.
         * @return Chunk count.
         */
        [[nodiscard]] USize GetChunkCount () const noexcept;

        /**
         * @brief Returns the number of retained object references.
         * @return Retained reference count.
         */
        [[nodiscard]] USize GetRetainedCount () const noexcept;

    private:

        struct FHeader
        {
            UInt32 Magic;
            UInt32 Opcode;
            UInt32 PayloadSize;
            UInt32 Reserved;
        };

        struct FRecord
        {
            USize ChunkIndex;
            USize Offset;
            UInt32 PayloadSize;
        };

        USize ChunkCapacity;
        USize CurrentChunk = 0U;

        TVector<TVector<Byte>> Chunks;
        TVector<FRecord> Records;
        TVector<TSharedPtr<IRHIObject>> RetainedObjects;
    };
} // namespace RHI

} // namespace Lumen
#include "Inline/RHICommandStream.inl"
