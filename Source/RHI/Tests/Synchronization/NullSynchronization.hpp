/**
 * @file NullSynchronization.hpp
 * @brief Declares deterministic Null semaphores.
 */

#pragma once

#include <HAL/PlatformMutex.hpp>
#include <RHI/Synchronization/IRHISemaphore.hpp>
#include <RHI/Types/RHIObjectForward.hpp>
#include <Templates/ConditionVariable.hpp>

namespace Lumen
{

namespace RHI
{

    namespace Tests
    {

        /**
         * @class FNullSemaphore
         * @brief Deterministic Null semaphore implementation.
         */
        class FNullSemaphore final : public IRHISemaphore
        {
        public:

            FNullSemaphore ( const FRHISemaphoreDesc &InDesc, FRHIObjectId InId ) noexcept;
            LUMEN_RHI_OBJECT_DECLARATIONS;
            const FRHISemaphoreDesc &GetDesc () const noexcept override;
            UInt64 GetValue () const noexcept override;
            FRHIExpected Signal ( UInt64 InValue ) noexcept override;
            FRHIExpected Wait ( UInt64 InValue, Float64 InTimeoutSeconds ) noexcept override;
            /** @brief Arms or clears a one-shot fault that rejects the next Signal call. */
            void SetFailNextSignal ( Bool bInFailNextSignal ) noexcept;

        private:

            FRHIObjectBase Identity;
            FRHISemaphoreDesc Desc;
            UInt64 Value            = 0ULL;
            UInt32 SignalFaultCount = 0U;
            mutable HAL::FPlatformRecursiveSharedMutex Mutex;
            FConditionVariable Condition;
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
