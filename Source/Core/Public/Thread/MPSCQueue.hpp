/**
 * @file MPSCQueue.hpp
 * @brief Bounded multi-producer single-consumer queue with preallocated storage.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/UniquePtr.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

#include <type_traits>

namespace Lumen
{

namespace Thread
{

    /**
     * @enum EMPSCQueueError
     * @brief Failure reported by TMPSCQueue initialization or insertion.
     */
    enum class EMPSCQueueError : UInt8
    {
        InvalidCapacity,
        AlreadyInitialized,
        NotInitialized,
        OutOfMemory,
        QueueFull,
    };

    /**
     * @class TMPSCQueue
     * @brief Bounded intrusive multi-producer single-consumer queue.
     * @tparam Type Payload type. Must be nothrow move-constructible and nothrow-destructible.
     * @details Dmitry Vyukov's intrusive MPSC list is paired with a bounded SPMC free-list. Initialize allocates exactly
     *          InCapacity + 1 nodes: one consumed-head stub and InCapacity message nodes. Push, Pop and node recycling never
     *          allocate or release storage; final queue destruction releases it. Structural producer operations are lock-free;
     *          the structural consumer path may briefly yield while a producer finishes publishing a claimed free-list slot.
     *          Payload operations retain the payload type's semantics.
     */
    template <typename Type> class TMPSCQueue final : public FNonCopyable, public FNonMovable
    {
        static_assert( std::is_nothrow_move_constructible_v<Type>, "TMPSCQueue payloads must be nothrow move-constructible." );
        static_assert( std::is_nothrow_destructible_v<Type>, "TMPSCQueue payloads must be nothrow-destructible." );

    public:

        /** @brief Creates an empty queue without allocating storage. */
        TMPSCQueue () noexcept = default;

        /** @brief Destroys every node and any payload still enqueued. No producer or consumer may remain active. */
        ~TMPSCQueue () noexcept = default;

    public:

        /**
         * @brief Allocates and links all queue storage.
         * @param InCapacity Exact number of message slots; must be greater than zero.
         * @return Success, or InvalidCapacity, AlreadyInitialized or OutOfMemory. Failure preserves the previous state.
         * @pre Initialization is a serial control operation and must not race with any other queue operation.
         */
        [[nodiscard]] TExpected<void, EMPSCQueueError> Initialize ( USize InCapacity ) noexcept;

        /**
         * @brief Enqueues a copy of InData. Safe for concurrent producers.
         * @param InData Payload to copy.
         * @return Success, NotInitialized, or QueueFull. Failure leaves both the queue and InData unchanged.
         */
        [[nodiscard]] TExpected<void, EMPSCQueueError> Push ( const Type &InData ) noexcept
            requires std::is_nothrow_copy_constructible_v<Type>;

        /**
         * @brief Enqueues InData by move. Safe for concurrent producers.
         * @param InData Payload to move.
         * @return Success, NotInitialized, or QueueFull. Failure leaves both the queue and InData unchanged.
         */
        [[nodiscard]] TExpected<void, EMPSCQueueError> Push ( Type &&InData ) noexcept;

        /**
         * @brief Removes the oldest visible message.
         * @return The payload, or an empty optional when no message is visible or the queue is uninitialized.
         * @note A single consumer must serialize every Pop and IsEmpty call. A producer between its tail exchange and link
         *       store can make the queue transiently appear empty; this does not imply that earlier producers have finished.
         */
        [[nodiscard]] TOptional<Type> Pop () noexcept;

        /** @return True after successful initialization. */
        [[nodiscard]] Bool IsInitialized () const noexcept;

        /** @return The exact message capacity, or zero before initialization. */
        [[nodiscard]] USize GetCapacity () const noexcept;

        /**
         * @return True when no message is currently visible to the consumer, including before initialization.
         * @note Consumer-only; the same transient producer-gap caveat as Pop applies.
         */
        [[nodiscard]] Bool IsEmpty () const noexcept;

    private:

        struct alignas( 64 ) FNode
        {
            TAtomic<FNode *> Next = { nullptr };
            TOptional<Type> Data;
        };

        struct alignas( 16 ) FFreeCell
        {
            TAtomic<USize> Sequence = { 0ULL };
            FNode *FreeNode         = nullptr;
        };

        template <typename ArgumentType>
        [[nodiscard]] TExpected<void, EMPSCQueueError> PushImpl ( ArgumentType &&InData ) noexcept;

        [[nodiscard]] FNode *AcquireNode () noexcept;
        void RecycleHead ( FNode *InOldHead ) noexcept;

    private:

        TUniquePtr<FNode[]> Nodes;
        TUniquePtr<FFreeCell[]> FreeStorage;

        alignas( 64 ) USize Capacity = 0ULL;
        USize FreeCapacityMask       = 0ULL;
        FNode *Head                  = nullptr;

        alignas( 64 ) TAtomic<FNode *> Tail = { nullptr };

        alignas( 64 ) USize FreeEnqueuePosition          = 0ULL;
        alignas( 64 ) TAtomic<USize> FreeDequeuePosition = { 0ULL };

    }; // class TMPSCQueue

} // namespace Thread

} // namespace Lumen

#include "Inline/MPSCQueue.inl"
