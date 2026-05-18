/**
 * @file VulkanDeferredDestruction.cpp
 * @brief Deferred resource destruction for Vulkan RHI implementation.
 */

#include "Vulkan/VulkanDeferredDestruction.hpp"

#include <utility>

void LumenEngine::VulkanRHI::FDeferredDestructionQueue::Enqueue ( LumenEngine::TFunction<void()> &&InDeletor, const LumenEngine::UInt64 InFrameIndex )
{
    Queue.push_back( { std::move( InDeletor ), InFrameIndex + MaxFramesInFlight } );
}

void LumenEngine::VulkanRHI::FDeferredDestructionQueue::Tick ( const LumenEngine::UInt64 InFrameIndex )
{
    for ( auto It = Queue.begin(); It != Queue.end(); )
    {
        if ( InFrameIndex >= It->ReleaseFrameIndex )
        {
            It->Deletor();
            It = Queue.erase( It );
        }
        else
        {
            ++It;
        }
    }
}

void LumenEngine::VulkanRHI::FDeferredDestructionQueue::Shutdown ()
{
    for ( auto &Item : Queue )
    {
        Item.Deletor();
    }
    Queue.clear();
}
