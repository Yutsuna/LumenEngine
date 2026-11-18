/**
 * @file IRHICommandAllocator.hpp
 * @brief Declares command-list recycling.
 */

#pragma once

#include <RHI/Types/IRHIObject.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIQueueType.hpp>
#include <RHI/Types/RHITimeline.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class IRHICommandAllocator
     * @brief Recycles completed command lists for one queue type.
     */
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

} // namespace RHI

} // namespace Lumen
