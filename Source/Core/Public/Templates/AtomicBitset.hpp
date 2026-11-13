/**
 * @file AtomicBitset.hpp
 * @brief Thread-safe bitset implementation using atomic operations.
 */

#pragma once

#include <CoreExport.hpp>

#include <Concepts/IsInvocable.hpp>
#include <Core/CoreTypes.hpp>
#include <Memory/UniquePtr.hpp>

namespace Lumen
{

/**
 * @class FAtomicBitset
 * @brief A bitset that allows atomic set and clear operations.
 * Useful for dirty tracking in multi-threaded environments.
 *
 * @details Memory ordering: Set() is a release operation and ForEachSetBitAndClear() is an
 * acquire operation, so data written before Set() is visible to the thread that consumes the
 * bit. Resize() is a structural change and must be externally synchronized against all access.
 * Indices passed to Set/Clear/Test must be smaller than GetNumBits().
 */
class CORE_EXPORT FAtomicBitset final
{
public:

    FAtomicBitset () noexcept = default;
    explicit FAtomicBitset ( USize InNumBits );

public:

    /** @brief Resizes the bitset. Structural change; clears all bits. Not thread-safe. */
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
        requires Concepts::CIsInvocable<Callable, USize>
    void ForEachSetBitAndClear ( Callable &&InFunc ) noexcept;

    /** @brief Clears all bits in the bitset. */
    void ClearAll () noexcept;

    /** @return The number of bits the bitset was sized for. */
    [[nodiscard]] USize GetNumBits () const noexcept
    {
        return NumBits;
    }

private:

    static constexpr USize BitsPerBlock = 64U;
    static constexpr UInt64 One         = 1ULL;

private:

    TUniquePtr<TAtomic<UInt64>[]> Blocks;
    USize NumBits   = 0U;
    USize NumBlocks = 0U;
};

} // namespace Lumen

#include "Inline/AtomicBitset.inl"
