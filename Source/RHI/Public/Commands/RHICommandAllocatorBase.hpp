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

    explicit FRHICommandAllocatorBase ( ERHIQueueType InQueueType ) noexcept;
    [[nodiscard]] ERHIQueueType GetQueueType () const noexcept override;
    TExpected<void, FRHIError> Reset ( FRHITimelineValue InCompleted ) noexcept override;
    /** @brief Adds/removes a list owned by the concrete backend allocator. */
    void RegisterList ( IRHICommandList &InList ) noexcept;
    void UnregisterList ( IRHICommandList &InList ) noexcept;
    [[nodiscard]] USize GetTrackedListCount () const noexcept;

private:

    ERHIQueueType QueueType;
    TVector<IRHICommandList *> Lists;
    mutable HAL::FPlatformRecursiveSharedMutex Mutex;
};
} // namespace Lumen
