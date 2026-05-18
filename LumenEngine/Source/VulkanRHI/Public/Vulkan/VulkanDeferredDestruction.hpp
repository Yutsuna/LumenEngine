/**
 * @file VulkanDeferredDestruction.hpp
 * @brief Deferred resource destruction for Vulkan RHI
 */

#pragma once

#include "Container/Function.hpp"
#include "Container/Vector.hpp"
#include "CoreTypes.hpp"
#include "Vulkan/VulkanCore.hpp"

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FDeferredDestructionQueue
     * @brief Delays resource destruction until the GPU is done with them.
     */
    class FDeferredDestructionQueue final
    {
    public:

        FDeferredDestructionQueue ()  = default;
        ~FDeferredDestructionQueue () = default;

    public:

        /**
         * @brief Enqueues a resource for destruction.
         * @param InDeletor   The function to call to destroy the resource.
         * @param InFrameIndex The current absolute frame index.
         */
        void Enqueue ( TFunction<void ()> &&InDeletor, UInt64 InFrameIndex );

        /**
         * @brief Processes the queue and destroys resources that are safe to release.
         * @param InFrameIndex The current absolute frame index.
         */
        void Tick ( UInt64 InFrameIndex );

        /**
         * @brief Immediately destroys all resources in the queue.
         */
        void Shutdown ();

    private:

        struct FPendingDestruction
        {
            TFunction<void ()> Deletor;
            UInt64 ReleaseFrameIndex;
        };

        TVector<FPendingDestruction> Queue;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
