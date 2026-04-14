/**
 * @file SPMCQueue.hpp
 * @brief Lock-free Single-Producer Multi-Consumer queue for Lumen Engine.
 */

#pragma once

#include "Container/Optional.hpp"
#include "Container/UniquePtr.hpp"
#include "CoreTypes.hpp"

#include "NonCopyable.hpp"
#include "NonMovable.hpp"

#include <cassert>

namespace LumenEngine
{

namespace Parallel
{

    /**
     * @class TSPMCQueue
     * @brief A lock-free Single-Producer Multi-Consumer (SPMC) bounded queue.
     * @details Based on Dmitry Vyukov's bounded array queue, explicitly optimized for a single publisher.
     *          Wait-free for the producer; lock-free for consumers.
     *
     * @tparam Type The payload type. Must be default constructible and movable.
     */
    template <typename Type> class TSPMCQueue final : public FNonCopyable, public FNonMovable
    {
    public:

        /**
         * @brief Initializes the queue with a fixed capacity.
         * @param InCapacity The maximum number of elements. Must be a power of two.
         */
        explicit TSPMCQueue ( USize InCapacity );

        /**
         * @brief Destructor. Destroys the bounded buffer naturally.
         */
        ~TSPMCQueue () noexcept = default;

    public:

        /**
         * @brief Attempts to push an element to the queue.
         * @param InData The element to copy.
         * @return True if successful, false if the queue is full.
         */
        [[nodiscard]] Bool Push ( const Type &InData ) noexcept;

        /**
         * @brief Attempts to push an element to the queue via move semantics.
         * @param InData The element to move.
         * @return True if successful, false if the queue is full.
         */
        [[nodiscard]] Bool Push ( Type &&InData ) noexcept;

        /**
         * @brief Attempts to pop an element from the queue. Multi-thread safe.
         * @return A valid optional containing the element if successful, or an empty optional if empty.
         */
        [[nodiscard]] TOptional<Type> Pop () noexcept;

    private:

        struct alignas( 16 ) FCell
        {
            TAtomic<USize> Sequence;
            Type Data;
        };

        TUniquePtr<FCell[]> OwnedBuffer;

        alignas( 64 ) USize CapacityMask = 0ULL;
        FCell *Buffer                    = nullptr;

        alignas( 64 ) USize EnqueuePosition = 0ULL;
        alignas( 64 ) TAtomic<USize> DequeuePosition;

    }; // class TSPMCQueue

} // namespace Parallel

} // namespace LumenEngine

#include "Inline/SPMCQueue.inl"