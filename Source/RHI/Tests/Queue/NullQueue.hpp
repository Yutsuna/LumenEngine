/**
 * @file NullQueue.hpp
 * @brief Declares serialized deterministic Null queues.
 */

#pragma once

#include <Device/NullFaultPlan.hpp>
#include <HAL/PlatformMutex.hpp>
#include <Queue/IRHIQueue.hpp>
#include <Templates/ConditionVariable.hpp>
#include <Types/RHIObjectForward.hpp>

namespace Lumen
{

namespace Tests
{

    /**
     * @class FNullQueue
     * @brief A serialized deterministic Null queue.
     */
    class FNullQueue final : public IRHIQueue
    {
    public:

        FNullQueue ( ERHIQueueType InType, UInt32 InIndex, TSharedPtr<FNullFaultPlan> InFaultPlan, FRHIObjectId InId ) noexcept;
        LUMEN_RHI_OBJECT_DECLARATIONS;
        ERHIQueueType GetType () const noexcept override;
        FRHITimelineValue GetCompletedValue () const noexcept override;
        TExpected<void, FRHIError> Submit ( const FRHISubmitDesc &InDesc ) noexcept override;
        TExpected<void, FRHIError> WaitIdle () noexcept override;
        TExpected<void, FRHIError> Wait ( FRHITimelineValue InValue, Float64 InTimeoutSeconds ) noexcept override;
        void MarkLost () noexcept;
        [[nodiscard]] UInt64 GetSubmitCount () const noexcept;

    private:

        FRHIObjectBase Identity;
        ERHIQueueType Type;
        TSharedPtr<FNullFaultPlan> FaultPlan;
        FRHITimelineValue Completed;
        UInt64 SubmitCount = 0ULL;
        Bool bLost         = false;
        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
        FConditionVariable Condition;
    };

} // namespace Tests

} // namespace Lumen
