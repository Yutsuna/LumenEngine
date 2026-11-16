/**
 * @file RHICommandStream.hpp
 * @brief Declares aligned backend-neutral command storage.
 */

#pragma once
#include <Memory/SharedPtr.hpp>
#include <Memory/Span.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>
#include <Types/IRHIObject.hpp>
#include <Types/RHIError.hpp>
#include <type_traits>
namespace Lumen

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

    explicit FRHICommandStream ( USize InChunkCapacity = 4096U ) noexcept;
    /** @brief Appends a raw payload; a command never crosses a chunk boundary. */
    TExpected<void, FRHIError> WritePayload ( UInt32 InOpcode, TSpan<const Byte> InPayload ) noexcept;
    /** @brief Appends one trivially copyable payload. */
    template <typename PayloadType>
        requires std::is_trivially_copyable_v<PayloadType>
    TExpected<void, FRHIError> Write ( UInt32 InOpcode, const PayloadType &InPayload ) noexcept;
    /** @brief Reads and validates one command by index. */
    [[nodiscard]] TExpected<FRHICommandView, FRHIError> Read ( USize InIndex ) const noexcept;
    /** @brief Decodes one typed command after checking opcode and payload size. */
    template <typename PayloadType>
        requires std::is_trivially_copyable_v<PayloadType>
    [[nodiscard]] TExpected<PayloadType, FRHIError> ReadPayload ( USize InIndex, UInt32 InExpectedOpcode ) const noexcept;
    /** @brief Retains an object until Clear or stream destruction. */
    void Retain ( TSharedPtr<IRHIObject> InObject ) noexcept;
    template <typename ObjectType> void Retain ( const TSharedRef<ObjectType> &InObject ) noexcept;
    void Clear () noexcept;
    [[nodiscard]] USize GetCommandCount () const noexcept;
    [[nodiscard]] USize GetChunkCount () const noexcept;
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
} // namespace Lumen
#include "Inline/RHICommandStream.inl"
