/**
 * @file AtomicBitset.hpp
 * @brief A thread-safe bitset implementation using atomic operations.
 */

#pragma once

#include "Container/UniquePtr.hpp"
#include "CoreTypes.hpp"

namespace LumenEngine
{

/**
 * @class FAtomicBitset
 * @brief A bitset that allows atomic set and clear operations.
 * Useful for dirty tracking in multi-threaded environments.
 */
class FAtomicBitset final
{
public:

    FAtomicBitset () noexcept = default;
    explicit FAtomicBitset ( USize InNumBits );

public:

    /** @brief Resizes the bitset. Structural change; clears all bits. */
    void Resize ( USize InNumBits );

    /** @brief Sets the bit at the specified index atomically. Thread-safe. */
    void Set ( USize InIndex ) noexcept;

    /** @brief Clears the bit at the specified index atomically. Thread-safe. */
    void Clear ( USize InIndex ) noexcept;

    /** @brief Tests if the bit at the specified index is set. Thread-safe. */
    [[nodiscard]] Bool Test ( USize InIndex ) const noexcept;

public:

    /**
     * @brief Iterates over all set bits, calls the provided function, and clears them.
     * The clearing is done per-block (64 bits) atomically using exchange(0).
     */
    template <typename Callable>
        requires std::is_invocable_v<Callable, USize>
    void ForEachSetBitAndClear ( Callable &&InFunc ) noexcept;

    /** @brief Clears all bits in the bitset. */
    void ClearAll () noexcept;

private:

    static constexpr USize BitsPerBlock = 64U;
    static constexpr UInt64 One         = 1ULL;

private:

    TUniquePtr<TAtomic<UInt64>[]> Blocks;
    USize NumBits   = 0U;
    USize NumBlocks = 0U;
};

} // namespace LumenEngine

#include "Inline/AtomicBitset.inl"