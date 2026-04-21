/**
 * @file LinearAllocator.hpp
 * @brief Simple and fast linear (arena) allocator for transient data.
 */

#pragma once

#include "CoreTypes.hpp"
#include "NonCopyable.hpp"
#include "NonMovable.hpp"

namespace LumenEngine
{

namespace HAL
{
    /**
     * @class FLinearAllocator
     * @brief A non-thread-safe allocator that allocates memory linearly from a pre-allocated block.
     *
     * This allocator is intended for high-performance transient data where allocations are
     * many and small, and can be cleared all at once with zero cost.
     *
     * @warning This allocator does NOT track object lifetimes. If you use New<T>(),
     * you are responsible for manually calling the destructor if T is not a POD.
     * Resetting or destroying the allocator will not call destructors for allocated objects.
     */
    class FLinearAllocator final : public FNonCopyable, public FNonMovable
    {
    public:

        /**
         * @brief Constructs a linear allocator wrapping an existing memory block.
         * @param InBytes Pointer to the memory block.
         * @param InSize Size of the memory block in bytes.
         */
        explicit FLinearAllocator ( void *InBytes, USize InSize ) noexcept;

        /**
         * @brief Default destructor. Does NOT free the wrapped buffer.
         */
        ~FLinearAllocator () noexcept = default;

    public:

        /**
         * @brief Allocates a block of memory.
         * @param InSize Size in bytes.
         * @param InAlignment Alignment in bytes (must be a power of 2).
         * @return Pointer to the allocated memory, or nullptr if out of memory.
         */
        [[nodiscard]] void *Allocate ( USize InSize, USize InAlignment = 16U ) noexcept;

        /**
         * @brief Resets the allocator offset to zero.
         */
        void Reset () noexcept;

        /**
         * @brief Helper to construct an object in the allocated memory.
         * @tparam ObjectType Type of the object.
         * @tparam TArgs Argument types for the constructor.
         * @param InArgs Arguments for the constructor.
         * @note If ObjectType owns non-trivial resources, callers must invoke its destructor manually.
         * @return Pointer to the constructed object, or nullptr if allocation failed.
         */
        template <typename ObjectType, typename... TArgs> [[nodiscard]] ObjectType *New ( TArgs &&...InArgs ) noexcept;

    public:

        /** @return Current memory usage in bytes. */
        [[nodiscard]] USize GetUsedMemory () const noexcept;

        /** @return Highest memory usage reached since allocator construction. */
        [[nodiscard]] USize GetHighWatermark () const noexcept;

        /** @return Total capacity in bytes. */
        [[nodiscard]] USize GetTotalMemory () const noexcept;

        /** @return Base address of the wrapped buffer. */
        [[nodiscard]] void *GetBaseAddress () const noexcept;

    private:

        /**
         * @brief Resets the allocator offset to a specific value.
         * @param InOffset The new offset.
         */
        void ResetTo ( USize InOffset ) noexcept;

    private:

        Byte *Buffer;
        USize TotalSize;
        USize Offset;
        USize HighWatermark;

        friend class FScopeLinear;

    }; // class FLinearAllocator

    /**
     * @class FScopeLinear
     * @brief RAII guard to automatically reset a linear allocator to its previous state.
     */
    class FScopeLinear final : public FNonCopyable, public FNonMovable
    {
    public:

        /**
         * @brief Captures the current state of the allocator.
         * @param InAllocator The allocator to guard.
         */
        explicit FScopeLinear ( FLinearAllocator &InAllocator ) noexcept;

        /**
         * @brief Restores the allocator to the captured state.
         */
        ~FScopeLinear () noexcept;

    private:

        FLinearAllocator &Allocator;
        USize PreviousOffset;

    }; // class FScopeLinear

} // namespace HAL

} // namespace LumenEngine

#include "Inline/LinearAllocator.inl"
