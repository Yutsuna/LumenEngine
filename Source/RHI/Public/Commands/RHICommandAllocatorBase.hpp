/**
 * @file RHICommandAllocatorBase.hpp
 * @brief Declares common command-list tracking and recycling.
 */

#pragma once
#include <Commands/IRHICommandAllocator.hpp>
#include <Commands/IRHICommandList.hpp>
#include <HAL/PlatformMutex.hpp>
#include <Templates/Vector.hpp>
namespace Lumen
{
class RHI_EXPORT FRHICommandAllocatorBase : public IRHICommandAllocator
{
public:

    /**
     * @brief Creates an allocator for a queue family.
     * @param InQueueType Family whose command lists this allocator tracks.
     */
    explicit FRHICommandAllocatorBase ( ERHIQueueType InQueueType ) noexcept;
    /**
     * @brief Returns the queue family fixed at construction.
     * @return The queue type.
     */
    [[nodiscard]] ERHIQueueType GetQueueType () const noexcept override;
    /**
     * @brief Recycles completed lists without disturbing pending ones.
     * @param InCompleted Timeline value known to have completed.
     * @return Success, or InvalidState if a tracked list is still pending above InCompleted.
     */
    TExpected<void, FRHIError> Reset ( FRHITimelineValue InCompleted ) noexcept override;
    /** @brief Adds/removes a list owned by the concrete backend allocator. */
    void RegisterList ( IRHICommandList &InList ) noexcept;
    /**
     * @brief Stops tracking a list, called from its destructor.
     * @param InList List being destroyed.
     */
    void UnregisterList ( IRHICommandList &InList ) noexcept;
    /**
     * @brief Returns the number of currently tracked live lists.
     * @return Tracked list count.
     */
    [[nodiscard]] USize GetTrackedListCount () const noexcept;

private:

    ERHIQueueType QueueType;
    TVector<IRHICommandList *> Lists;
    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
};
} // namespace Lumen
