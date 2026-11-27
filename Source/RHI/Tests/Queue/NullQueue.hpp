/**
 * @file NullQueue.hpp
 * @brief Declares serialized deterministic Null queues.
 */

#pragma once

#include <Device/NullFaultPlan.hpp>
#include <HAL/PlatformMutex.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Types/RHIObjectForward.hpp>
#include <Templates/ConditionVariable.hpp>

namespace Lumen
{

namespace RHI
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

            FNullQueue ( ERHIQueueType InType,
                         UInt32 InIndex,
                         TSharedPtr<FNullFaultPlan> InFaultPlan,
                         FRHIObjectId InId ) noexcept;
            LUMEN_RHI_OBJECT_DECLARATIONS;
            ERHIQueueType GetType () const noexcept override;
            FRHITimelineValue GetCompletedValue () const noexcept override;
            FRHIExpected Submit ( const FRHISubmitDesc &InDesc ) noexcept override;
            FRHIExpected WaitIdle () noexcept override;
            FRHIExpected Wait ( FRHITimelineValue InValue, Float64 InTimeoutSeconds ) noexcept override;
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

} // namespace RHI

} // namespace Lumen
