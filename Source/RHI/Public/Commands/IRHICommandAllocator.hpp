/**
 * @file IRHICommandAllocator.hpp
 * @brief Declares command-list recycling.
 */

#pragma once
#include <Types/IRHIObject.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIQueueType.hpp>
#include <Types/RHITimeline.hpp>
namespace Lumen
{
class IRHICommandAllocator : public IRHIObject
{
public:

    /**
     * @brief Returns the queue family whose lists this allocator recycles.
     * @return The queue type fixed at creation.
     */
    [[nodiscard]] virtual ERHIQueueType GetQueueType () const noexcept = 0;
    /** @brief Resets completed lists; pending work above InCompleted prevents recycling. */
    virtual TExpected<void, FRHIError> Reset ( FRHITimelineValue InCompleted ) noexcept = 0;
};
} // namespace Lumen
