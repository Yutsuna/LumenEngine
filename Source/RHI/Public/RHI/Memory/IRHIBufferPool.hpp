/**
 * @file IRHIBufferPool.hpp
 * @brief Declares buffer recycling by compatible descriptor.
 */
#pragma once

#include <Memory/SharedPtr.hpp>
#include <RHI/Types/IRHIObject.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHITimeline.hpp>

namespace Lumen
{

namespace RHI
{

    class IRHIBuffer;
    struct FRHIBufferDesc;

    /**
     * @class IRHIBufferPool
     * @brief Device-created pool of reusable buffers.
     */
    class IRHIBufferPool : public IRHIObject
    {
    public:

        /** @brief Acquires a buffer matching InDesc. */
        [[nodiscard]] virtual TExpected<TSharedRef<IRHIBuffer>, FRHIError> Acquire ( const FRHIBufferDesc &InDesc ) noexcept = 0;
        /** @brief Returns a buffer for reuse after its completion value. */
        [[nodiscard]] virtual TExpected<void, FRHIError> Release ( TSharedRef<IRHIBuffer> InBuffer,
                                                                   FRHITimelineValue InCompletion ) noexcept = 0;
        /** @brief Publishes the current GPU completion value so finished entries become reusable. */
        virtual void AdvanceCompleted ( FRHITimelineValue InCompleted ) noexcept = 0;
        /** @return Number of released entries still waiting for their completion value. */
        [[nodiscard]] virtual USize GetPendingCount () const noexcept = 0;
    };
} // namespace RHI

} // namespace Lumen
