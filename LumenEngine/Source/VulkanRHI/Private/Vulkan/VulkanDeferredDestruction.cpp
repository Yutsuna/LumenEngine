/**
 * @file VulkanDeferredDestruction.cpp
 * @brief Deferred resource destruction for Vulkan RHI implementation.
 */

#include "Vulkan/VulkanDeferredDestruction.hpp"

#include <algorithm>
#include <utility>

void LumenEngine::VulkanRHI::FDeferredDestructionQueue::Enqueue ( LumenEngine::TFunction<void()> &&InDeleter, const LumenEngine::UInt64 InFrameIndex )
{
    Queue.push_back( { std::move( InDeleter ), InFrameIndex + MaxFramesInFlight } );
}

void LumenEngine::VulkanRHI::FDeferredDestructionQueue::Tick ( const LumenEngine::UInt64 InFrameIndex )
{
    /** INFO: Since resources are enqueued with monotonically increasing frame indices, the queue is naturally sorted. */
    auto It = Queue.begin();
    while ( It != Queue.end() and InFrameIndex >= It->ReleaseFrameIndex )
    {
        It->Deleter();
        ++It;
    }

    if ( It != Queue.begin() )
    {
        Queue.erase( Queue.begin(), It );
    }
}

void LumenEngine::VulkanRHI::FDeferredDestructionQueue::Shutdown ()
{
    for ( auto &Item : Queue )
    {
        Item.Deleter();
    }
    Queue.clear();
}
