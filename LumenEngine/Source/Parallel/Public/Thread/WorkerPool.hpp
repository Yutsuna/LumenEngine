/**
 * @file WorkerPool.hpp
 * @brief High-performance thread pool using a lock-free SPMC queue.
 */

#pragma once

#include "Container/Vector.hpp"

#include "NonCopyable.hpp"
#include "NonMovable.hpp"

#include "HAL/Memory/LinearAllocator.hpp"
#include "Thread/SPMCQueue.hpp"

#include <functional>
#include <semaphore>
#include <thread>

namespace LumenEngine
{

namespace Parallel
{

    /**
     * @class FWorkerPool
     * @brief Pre-allocated pool of persistent workers processing a centralized lock-free queue.
     */
    class FWorkerPool final : public FNonCopyable, public FNonMovable
    {
    public:

        using FTask = std::function<void()>;

        /**
         * @brief Initializes the pool and spins up the workers.
         * @param InWorkerCount The number of hardware threads to allocate.
         * @param InQueueCapacity The maximum number of pending tasks (will be rounded up to the nearest power of 2).
         */
        explicit FWorkerPool ( UInt32 InWorkerCount, USize InQueueCapacity = 1024ULL ) noexcept;

        /**
         * @brief Cleans up workers gracefully. Calls Shutdown() automatically.
         */
        ~FWorkerPool () noexcept;

    public:

        /**
         * @brief Thread-safe submission of a task to the queue.
         * @param InTask The lambda or functor to execute.
         * @return True if accepted, false if the queue is full or the pool is shutting down.
         */
        Bool Submit ( FTask InTask ) noexcept;

        /**
         * @brief Blocks the calling thread until all currently dispatched tasks have finished executing.
         */
        void WaitAll () noexcept;

        /**
         * @brief Signals all workers to cease operations immediately and joins their threads.
         */
        void Shutdown () noexcept;

        /**
         * @brief Returns the current worker index when called from a worker thread.
         * @return Worker index bound to the calling thread, or empty if called from outside the pool.
         */
        static TOptional<UInt32> GetCurrentWorkerIndex () noexcept;

        /**
         * @brief Returns the linear allocator for the current worker thread.
         * @return Reference to the worker's linear allocator.
         * @note Only valid when called from a worker thread. Behavior is undefined otherwise.
         */
        static HAL::FLinearAllocator &GetWorkerAllocator () noexcept;

        /**
         * @brief Snapshot of executed task count per worker.
         * @return A vector sized to worker count containing per-worker execution counters.
         */
        [[nodiscard]] TVector<UInt64> GetWorkerExecutionCounts () const noexcept;

        /**
         * @brief Total number of tasks executed since pool creation.
         * @return Sum of all worker execution counters.
         */
        [[nodiscard]] UInt64 GetTotalExecutedTaskCount () const noexcept;

    private:

        void WorkerLoop ( UInt32 InWorkerIndex ) noexcept;

    private:

        TSPMCQueue<FTask> Queue;
        TVector<std::jthread> Workers;

        std::counting_semaphore<> TaskSemaphore;
        TAtomic<UInt32> ActiveTaskCount;
        TAtomic<Bool> bIsActive;
        TVector<TAtomic<UInt64>> WorkerExecutionCounts;

    }; // class FWorkerPool

} // namespace Parallel

} // namespace LumenEngine