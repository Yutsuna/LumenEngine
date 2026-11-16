/**
 * @file NullFaultPlan.hpp
 * @brief Declares deterministic fault injection for the Null backend.
 */

#pragma once

#include <HAL/PlatformMutex.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>
#include <Types/RHIError.hpp>

namespace Lumen
{

namespace Tests
{

    /**
     * @class FNullFaultPlan
     * @brief Thread-safe operation/occurrence fault schedule.
     */
    class FNullFaultPlan final
    {
    public:

        /** @brief Fails the InOccurrence-th call of InOperation with InError; occurrence is one-based. */
        void AddFailure ( FStringView InOperation, UInt32 InOccurrence, const FRHIError &InError ) noexcept;
        /** @return The scheduled error when this call matches, otherwise empty. */
        [[nodiscard]] TOptional<FRHIError> ShouldFail ( FStringView InOperation ) noexcept;

    private:

        struct FRule
        {
            FString Operation;
            UInt32 Occurrence = 1U;
            UInt32 Calls      = 0U;
            FRHIError Error;
        };
        TVector<FRule> Rules;
        HAL::FPlatformRecursiveSharedMutex Mutex;
    };

} // namespace Tests

} // namespace Lumen
